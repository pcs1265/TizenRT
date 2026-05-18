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

	memset(vq, 0, sizeof(virtq_t));
}

/****************************************************************************
 * Name: virtq_add_buffer
 *
 * Description:
 *   Add a chained descriptor buffer to the virtqueue
 *
 ****************************************************************************/

int virtq_add_buffer_cookie(virtq_t *vq, struct virtq_desc *descs,
			    uint16_t ndesc, void *cookie)
{
	uint16_t i;
	uint16_t head;
	uint16_t slot;
	uint16_t avail_ring_idx;

	if (!vq || !vq->ready || !descs || ndesc == 0) {
		return -EINVAL;
	}

	head = vq->free_head;
	vq->cookie[head] = cookie;

	/* Fill descriptor table slots with the chained descriptors */

	for (i = 0; i < ndesc; i++) {
		slot = (head + i) & vq->num_mask;
		vq->desc[slot].addr  = descs[i].addr;
		vq->desc[slot].len   = descs[i].len;
		vq->desc[slot].flags = descs[i].flags;

		if (i < ndesc - 1) {
			vq->desc[slot].flags |= VIRTQ_DESC_F_NEXT;
			vq->desc[slot].next   = (head + i + 1) & vq->num_mask;
		} else {
			vq->desc[slot].flags &= ~VIRTQ_DESC_F_NEXT;
			vq->desc[slot].next   = 0;
		}
	}

	/* Add head descriptor index to the available ring */

	avail_ring_idx = vq->avail->idx & vq->num_mask;
	vq->avail->ring[avail_ring_idx] = head;

	__sync_synchronize();	/* Ensure descriptors are visible before idx update */
	vq->avail->idx++;

	/* Advance free_head; safe because we wait for completion before reuse */

	vq->free_head = (head + ndesc) & vq->num_mask;

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
	uint16_t used_idx;

	if (!vq || !vq->ready) {
		return -EINVAL;
	}

	/* Check if there are used buffers */
	if (vq->last_used_idx == vq->used->idx) {
		return -EAGAIN;	/* No used buffers available */
	}

	/* Get the used index */
	used_idx = vq->last_used_idx & vq->num_mask;

	/* Get the buffer length */
	if (len) {
		*len = vq->used->ring[used_idx].len;
	}

	/* Update last used index */
	vq->last_used_idx++;

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
	void *cookie;

	if (!vq || !vq->ready) {
		return NULL;
	}

	if (vq->last_used_idx == vq->used->idx) {
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

	vq->last_used_idx++;
	return cookie;
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
 * Name: virtq_kick
 *
 * Description:
 *   Notify the device about added buffers
 *
 ****************************************************************************/

void virtq_kick(virtq_t *vq)
{
	if (!vq || !vq->ready) {
		return;
	}

	/* TODO: Notify the device (this would typically involve writing to a register) */
	/* For now, we'll just log that kick was called */
	// vdbg("Virtqueue kicked\n");
}
