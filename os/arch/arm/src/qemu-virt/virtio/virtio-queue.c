/****************************************************************************
 *
 * Copyright 2025 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <tinyara/arch.h>
#include <tinyara/kmalloc.h>
#include <tinyara/spinlock.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include "virtio-queue.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: virtq_alloc_memory
 *
 * Description:
 *   Allocate memory for virtqueue descriptors, available ring and used ring
 *
 ****************************************************************************/

static int virtq_alloc_memory(virtq_t *vq)
{
	size_t desc_size, avail_size, used_size;
	uint8_t *memory;
	uint32_t page_size = 4096;	/* Assuming 4KB page size */

	/* Calculate memory requirements */

	desc_size = vq->num * sizeof(struct virtq_desc);
	avail_size = sizeof(uint16_t) * (3 + vq->num);
	used_size = sizeof(uint16_t) * 3 + sizeof(struct virtq_used_elem) * vq->num;

	/* Align to page size */

	desc_size = (desc_size + page_size - 1) & ~(page_size - 1);
	avail_size = (avail_size + page_size - 1) & ~(page_size - 1);
	used_size = (used_size + page_size - 1) & ~(page_size - 1);

	/* Allocate with extra room to guarantee 4096-byte alignment.
	 * v1 MMIO queue setup (QUEUE_PFN) requires the descriptor table to
	 * sit at a page-aligned physical address.
	 */

	vq->raw_mem = kmm_malloc(desc_size + avail_size + used_size + (page_size - 1));
	if (!vq->raw_mem) {
		return -ENOMEM;
	}
	memory = (uint8_t *)(((uintptr_t)vq->raw_mem + (page_size - 1)) & ~((uintptr_t)(page_size - 1)));

	/* Set up pointers */

	vq->desc = (struct virtq_desc *)memory;
	vq->avail = (struct virtq_avail *)(memory + desc_size);
	vq->used = (struct virtq_used *)(memory + desc_size + avail_size);

	/* Initialize memory */

	memset(vq->desc, 0, desc_size);
	memset(vq->avail, 0, avail_size);
	memset(vq->used, 0, used_size);

	return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: virtq_init
 *
 * Description:
 *   Initialize a virtqueue
 *
 ****************************************************************************/

int virtq_init(virtq_t *vq, uint16_t num)
{
	uint16_t i;
	int ret;

	/* Validate input parameters */

	if (!vq || num == 0 || (num & (num - 1)) != 0) {
		/* num must be a power of 2 */
		return -EINVAL;
	}

	/* Initialize virtqueue structure */

	memset(vq, 0, sizeof(virtq_t));
	vq->num = num;
	vq->num_mask = num - 1;
	vq->num_free = num;
	vq->free_head = 0;
	vq->vq_nentries = num;

	/* Initialize spinlock to unlocked state */

	spin_initialize(&vq->lock, SP_UNLOCKED);

	/* Allocate memory for queue */

	ret = virtq_alloc_memory(vq);
	if (ret != OK) {
		return ret;
	}

	vq->cookie = kmm_zalloc(num * sizeof(void *));
	if (vq->cookie == NULL) {
		virtq_deinit(vq);
		return -ENOMEM;
	}

	/* Allocate array to track descriptor chain lengths */

	vq->desc_chain_len = kmm_zalloc(num * sizeof(uint16_t));
	if (vq->desc_chain_len == NULL) {
		virtq_deinit(vq);
		return -ENOMEM;
	}

	/* Initialize the free descriptor list by chaining all descriptors.
	 * The free list is a linked list using desc[i].next pointers.
	 * After reclamation, this list may become non-contiguous, so
	 * allocation must walk the chain via next pointers.
	 */

	for (i = 0; i < num - 1; i++) {
		vq->desc[i].flags = VIRTQ_DESC_F_NEXT;
		vq->desc[i].next = i + 1;
	}
	vq->desc[num - 1].flags = 0;
	vq->desc[num - 1].next = 0;

	vq->ready = true;
	return OK;
}

/****************************************************************************
 * Name: virtq_deinit
 *
 * Description:
 *   Deinitialize a virtqueue
 *
 ****************************************************************************/

void virtq_deinit(virtq_t *vq)
{
	if (!vq) {
		return;
	}

	if (vq->raw_mem) {
		kmm_free(vq->raw_mem);
	}

	if (vq->cookie) {
		kmm_free(vq->cookie);
	}

	if (vq->desc_chain_len) {
		kmm_free(vq->desc_chain_len);
	}

	memset(vq, 0, sizeof(virtq_t));
}

/****************************************************************************
 * Name: virtq_add_buffer_cookie
 *
 * Description:
 *   Add a chained descriptor buffer to the virtqueue.
 *   Walks the free chain via next pointers instead of assuming contiguous
 *   layout, which is essential after descriptor reclamation creates gaps.
 *
 ****************************************************************************/

int virtq_add_buffer_cookie(virtq_t *vq, struct virtq_desc *descs,
			    uint16_t ndesc, void *cookie)
{
	uint16_t i;
	uint16_t head;
	uint16_t slot;
	uint16_t avail_ring_idx;
	uint16_t next_free;
	uint16_t chain_slots[VIRTQ_DESC_MAX];
	irqstate_t flags;

	if (!vq || !vq->ready || !descs || ndesc == 0) {
		return -EINVAL;
	}

	if (ndesc > VIRTQ_DESC_MAX) {
		return -EINVAL;
	}

	/* Acquire spinlock with interrupt disable to prevent ISR concurrency */

	flags = spin_lock_irqsave(&vq->lock);

	/* Check if we have enough free descriptors */

	if (vq->num_free < ndesc) {
		spin_unlock_irqrestore(&vq->lock, flags);
		return -ENOSPC;
	}

	head = vq->free_head;
	vq->cookie[head] = cookie;
	vq->desc_chain_len[head] = ndesc;  /* Track chain length for reclamation */

	/* Step 1: Walk the free chain via next pointers to record slot order.
	 * We cannot assume contiguous layout because reclamation creates gaps
	 * in the free list (e.g., 0->1->2->5->6->7 after reclaiming 0,1,2).
	 * We must save the next-pointers BEFORE overwriting any descriptors.
	 */

	slot = head;
	for (i = 0; i < ndesc; i++) {
		chain_slots[i] = slot;
		if (i < ndesc - 1) {
			slot = vq->desc[slot].next;
		}
	}

	/* The next free descriptor after the allocated chain is the next
	 * pointer of the last descriptor in the chain (still unmodified).
	 */

	next_free = vq->desc[chain_slots[ndesc - 1]].next;

	/* Step 2: Fill descriptor table slots with the caller's data and
	 * set up the chain links using the recorded slot order.
	 */

	for (i = 0; i < ndesc; i++) {
		slot = chain_slots[i];
		vq->desc[slot].addr  = descs[i].addr;
		vq->desc[slot].len   = descs[i].len;
		vq->desc[slot].flags = descs[i].flags;

		if (i < ndesc - 1) {
			vq->desc[slot].flags |= VIRTQ_DESC_F_NEXT;
			vq->desc[slot].next   = chain_slots[i + 1];
		} else {
			vq->desc[slot].flags &= ~VIRTQ_DESC_F_NEXT;
			vq->desc[slot].next   = 0;
		}
	}

	/* Step 3: Add head descriptor index to the available ring */

	avail_ring_idx = vq->avail->idx & vq->num_mask;
	vq->avail->ring[avail_ring_idx] = head;

	__sync_synchronize();	/* Ensure descriptors and avail ring are visible before idx update */
	vq->avail->idx++;

	/* Step 4: Update free list: advance free_head to the first free
	 * descriptor after the allocated chain.
	 */

	vq->free_head = next_free;
	vq->num_free -= ndesc;

	spin_unlock_irqrestore(&vq->lock, flags);

	return OK;
}

/****************************************************************************
 * Name: virtqueue_add_buffer
 *
 * Description:
 *   NuttX-style helper to add readable and writable buffers with a cookie.
 *
 ****************************************************************************/

int virtqueue_add_buffer(struct virtqueue *vq, struct virtqueue_buf *buf_list,
			 int readable, int writable, void *cookie)
{
	struct virtq_desc descs[VIRTQ_DESC_MAX];
	uint16_t ndesc;
	uint16_t i;

	if (!buf_list || readable < 0 || writable < 0) {
		return -EINVAL;
	}

	ndesc = (uint16_t)(readable + writable);
	if (ndesc == 0 || ndesc > VIRTQ_DESC_MAX) {
		return -EINVAL;
	}

	for (i = 0; i < ndesc; i++) {
		descs[i].addr = (uint64_t)(uintptr_t)buf_list[i].buf;
		descs[i].len = buf_list[i].len;
		descs[i].flags = i >= readable ? VIRTQ_DESC_F_WRITE : 0;
		descs[i].next = 0;
	}

	return virtq_add_buffer_cookie(vq, descs, ndesc, cookie);
}

/****************************************************************************
 * Name: virtqueue_add_buffer_lock
 ****************************************************************************/

int virtqueue_add_buffer_lock(struct virtqueue *vq,
			      struct virtqueue_buf *buf_list, int readable,
			      int writable, void *cookie, spinlock_t *lock)
{
	irqstate_t flags;
	int ret;

	if (vq != NULL && lock == &vq->lock) {
		return virtqueue_add_buffer(vq, buf_list, readable, writable,
					    cookie);
	}

	flags = spin_lock_irqsave(lock);
	ret = virtqueue_add_buffer(vq, buf_list, readable, writable, cookie);
	spin_unlock_irqrestore(lock, flags);

	return ret;
}

/****************************************************************************
 * Name: virtq_add_buffer
 *
 * Description:
 *   Add a chained descriptor buffer to the virtqueue without a cookie.
 *
 ****************************************************************************/

int virtq_add_buffer(virtq_t *vq, struct virtq_desc *descs, uint16_t ndesc)
{
	return virtq_add_buffer_cookie(vq, descs, ndesc, NULL);
}

/****************************************************************************
 * Name: virtq_get_buffer
 *
 * Description:
 *   Get a used buffer from the virtqueue
 *
 ****************************************************************************/

int virtq_get_buffer(virtq_t *vq, uint32_t *len)
{
	struct virtq_used_elem *used;
	uint16_t used_idx;
	uint16_t head;
	uint16_t chain_len;
	uint16_t i;
	uint16_t last_desc;
	irqstate_t flags;

	if (!vq || !vq->ready) {
		return -EINVAL;
	}

	/* Acquire spinlock with interrupt disable */

	flags = spin_lock_irqsave(&vq->lock);

	/* Memory barrier to ensure we see updated used ring */

	__sync_synchronize();

	/* Check if there are used buffers */

	if (vq->last_used_idx == vq->used->idx) {
		spin_unlock_irqrestore(&vq->lock, flags);
		return -EAGAIN;	/* No used buffers available */
	}

	/* Get the used index */

	used_idx = vq->last_used_idx & vq->num_mask;
	used = &vq->used->ring[used_idx];
	head = (uint16_t)used->id;

	/* Get the buffer length */

	if (len) {
		*len = used->len;
	}

	/* Reclaim the descriptor chain back to the free list.
	 * Walk the chain via next pointers to find the last descriptor,
	 * then link it to the current free_head and set free_head to
	 * the reclaimed chain head.
	 */

	if (head < vq->num) {
		chain_len = vq->desc_chain_len[head];
	} else {
		chain_len = 0;
	}

	if (chain_len > 0) {
		/* Walk the chain to find the last descriptor */

		last_desc = head;
		for (i = 0; i < chain_len - 1; i++) {
			if (!(vq->desc[last_desc].flags & VIRTQ_DESC_F_NEXT)) {
				break;	/* Safety: chain shorter than expected */
			}
			last_desc = vq->desc[last_desc].next;
		}

		/* Link the last descriptor's next to current free_head */

		vq->desc[last_desc].flags = 0;
		vq->desc[last_desc].next = vq->free_head;

		/* Update free_head to point to the reclaimed chain head */

		vq->free_head = head;
		vq->num_free += chain_len;

		/* Clear chain length tracking */

		vq->desc_chain_len[head] = 0;
	}

	/* Update last used index */

	vq->last_used_idx++;

	spin_unlock_irqrestore(&vq->lock, flags);

	return OK;
}

/****************************************************************************
 * Name: virtq_get_buffer_cookie
 *
 * Description:
 *   Get a used buffer cookie from the virtqueue.
 *
 ****************************************************************************/

void *virtq_get_buffer_cookie(virtq_t *vq, uint32_t *len, uint16_t *idx)
{
	struct virtq_used_elem *used;
	uint16_t used_idx;
	uint16_t head;
	uint16_t chain_len;
	uint16_t i;
	uint16_t last_desc;
	void *cookie;
	irqstate_t flags;

	if (!vq || !vq->ready) {
		return NULL;
	}

	/* Acquire spinlock with interrupt disable */

	flags = spin_lock_irqsave(&vq->lock);

	/* Memory barrier to ensure we see updated used ring */

	__sync_synchronize();

	if (vq->last_used_idx == vq->used->idx) {
		spin_unlock_irqrestore(&vq->lock, flags);
		return NULL;
	}

	used_idx = vq->last_used_idx & vq->num_mask;
	used = &vq->used->ring[used_idx];
	head = (uint16_t)used->id;

	if (len) {
		*len = used->len;
	}

	if (idx) {
		*idx = head;
	}

	cookie = head < vq->num ? vq->cookie[head] : NULL;
	if (head < vq->num) {
		vq->cookie[head] = NULL;
	}

	/* Reclaim the descriptor chain back to the free list.
	 * Walk the chain via next pointers to find the last descriptor,
	 * then link it to the current free_head and set free_head to
	 * the reclaimed chain head.
	 */

	if (head < vq->num) {
		chain_len = vq->desc_chain_len[head];
	} else {
		chain_len = 0;
	}

	if (chain_len > 0) {
		/* Walk the chain to find the last descriptor */

		last_desc = head;
		for (i = 0; i < chain_len - 1; i++) {
			if (!(vq->desc[last_desc].flags & VIRTQ_DESC_F_NEXT)) {
				break;	/* Safety: chain shorter than expected */
			}
			last_desc = vq->desc[last_desc].next;
		}

		/* Link the last descriptor's next to current free_head */

		vq->desc[last_desc].flags = 0;
		vq->desc[last_desc].next = vq->free_head;

		/* Update free_head to point to the reclaimed chain head */

		vq->free_head = head;
		vq->num_free += chain_len;

		/* Clear chain length tracking */

		vq->desc_chain_len[head] = 0;
	}

	vq->last_used_idx++;

	spin_unlock_irqrestore(&vq->lock, flags);

	return cookie;
}

/****************************************************************************
 * Name: virtqueue_nused
 *
 * Description:
 *   Return the number of used buffers visible to the driver.
 *
 ****************************************************************************/

uint16_t virtqueue_nused(struct virtqueue *vq)
{
	if (!vq || !vq->ready) {
		return 0;
	}

	__sync_synchronize();
	return (uint16_t)(vq->used->idx - vq->last_used_idx);
}

/****************************************************************************
 * Name: virtqueue_set_callback
 ****************************************************************************/

void virtqueue_set_callback(struct virtqueue *vq, vq_callback callback)
{
	irqstate_t flags;

	if (!vq) {
		return;
	}

	flags = spin_lock_irqsave(&vq->lock);
	vq->callback_saved = callback;
	if (vq->cb_enabled) {
		vq->callback = callback;
	}

	spin_unlock_irqrestore(&vq->lock, flags);
}

/****************************************************************************
 * Name: virtqueue_set_notify
 ****************************************************************************/

void virtqueue_set_notify(struct virtqueue *vq, virtqueue_notify_t notify,
			  void *arg)
{
	irqstate_t flags;

	if (!vq) {
		return;
	}

	flags = spin_lock_irqsave(&vq->lock);
	vq->notify = notify;
	vq->notify_arg = arg;
	spin_unlock_irqrestore(&vq->lock, flags);
}

/****************************************************************************
 * Name: virtqueue_enable_cb
 ****************************************************************************/

void virtqueue_enable_cb(struct virtqueue *vq)
{
	irqstate_t flags;

	if (!vq || !vq->ready) {
		return;
	}

	flags = spin_lock_irqsave(&vq->lock);
	vq->avail->flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;
	vq->cb_enabled = true;
	vq->callback = vq->callback_saved;
	__sync_synchronize();
	spin_unlock_irqrestore(&vq->lock, flags);
}

/****************************************************************************
 * Name: virtqueue_disable_cb
 ****************************************************************************/

void virtqueue_disable_cb(struct virtqueue *vq)
{
	irqstate_t flags;

	if (!vq || !vq->ready) {
		return;
	}

	flags = spin_lock_irqsave(&vq->lock);
	vq->avail->flags |= VIRTQ_AVAIL_F_NO_INTERRUPT;
	vq->cb_enabled = false;
	vq->callback = NULL;
	__sync_synchronize();
	spin_unlock_irqrestore(&vq->lock, flags);
}

/****************************************************************************
 * Name: virtqueue_enable_cb_lock
 ****************************************************************************/

void virtqueue_enable_cb_lock(struct virtqueue *vq, spinlock_t *lock)
{
	irqstate_t flags;

	if (vq != NULL && lock == &vq->lock) {
		virtqueue_enable_cb(vq);
		return;
	}

	flags = spin_lock_irqsave(lock);
	virtqueue_enable_cb(vq);
	spin_unlock_irqrestore(lock, flags);
}

/****************************************************************************
 * Name: virtqueue_disable_cb_lock
 ****************************************************************************/

void virtqueue_disable_cb_lock(struct virtqueue *vq, spinlock_t *lock)
{
	irqstate_t flags;

	if (vq != NULL && lock == &vq->lock) {
		virtqueue_disable_cb(vq);
		return;
	}

	flags = spin_lock_irqsave(lock);
	virtqueue_disable_cb(vq);
	spin_unlock_irqrestore(lock, flags);
}

/****************************************************************************
 * Name: virtqueue_get_buffer
 *
 * Description:
 *   NuttX-style helper to get a used buffer cookie from the virtqueue.
 *
 ****************************************************************************/

void *virtqueue_get_buffer(struct virtqueue *vq, uint32_t *len, uint16_t *idx)
{
	return virtq_get_buffer_cookie(vq, len, idx);
}

/****************************************************************************
 * Name: virtqueue_get_buffer_lock
 ****************************************************************************/

void *virtqueue_get_buffer_lock(struct virtqueue *vq, uint32_t *len,
				uint16_t *idx, spinlock_t *lock)
{
	irqstate_t flags;
	void *cookie;

	if (vq != NULL && lock == &vq->lock) {
		return virtqueue_get_buffer(vq, len, idx);
	}

	flags = spin_lock_irqsave(lock);
	cookie = virtqueue_get_buffer(vq, len, idx);
	spin_unlock_irqrestore(lock, flags);

	return cookie;
}

/****************************************************************************
 * Name: virtqueue_kick
 *
 * Description:
 *   NuttX-style queue kick wrapper.
 *
 ****************************************************************************/

void virtqueue_kick(struct virtqueue *vq)
{
	virtq_kick(vq);
}

/****************************************************************************
 * Name: virtqueue_kick_lock
 ****************************************************************************/

void virtqueue_kick_lock(struct virtqueue *vq, spinlock_t *lock)
{
	irqstate_t flags;

	if (vq != NULL && lock == &vq->lock) {
		virtqueue_kick(vq);
		return;
	}

	flags = spin_lock_irqsave(lock);
	virtqueue_kick(vq);
	spin_unlock_irqrestore(lock, flags);
}

/****************************************************************************
 * Name: virtq_kick
 *
 * Description:
 *   Notify the device about added buffers
 *
 ****************************************************************************/

void virtq_kick(virtq_t *vq)
{
	virtqueue_notify_t notify;
	void *notify_arg;

	if (!vq || !vq->ready) {
		return;
	}

	__sync_synchronize();

	notify = vq->notify;
	notify_arg = vq->notify_arg;
	if (notify != NULL) {
		notify(vq, notify_arg);
	}
}
