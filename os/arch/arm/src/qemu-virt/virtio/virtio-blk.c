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
#include <tinyara/irq.h>
#include <tinyara/semaphore.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include "virtio-blk.h"
#include <stdlib.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: virtio_blk_interrupt
 *
 * Description:
 *   Interrupt handler for virtio block device
 *
 ****************************************************************************/

static int virtio_blk_interrupt(int irq, FAR void *context, FAR void *arg)
{
	virtio_blk_dev_t *dev = (virtio_blk_dev_t *)arg;
	uint32_t int_status;

	/* Get interrupt status */

	int_status = virtio_mmio_get_interrupt_status(&dev->mmio_dev);

	/* Acknowledge interrupt */

	virtio_mmio_interrupt_ack(&dev->mmio_dev, int_status);

	/* Handle interrupt */

	if (int_status & VIRTIO_MMIO_INT_VRING) {
		/* VRING interrupt - device has used a buffer */

		/* Set completion flag */

		dev->completion_received = true;

		/* Wake up waiting task */

		sem_post(&dev->completion_sem);
	}

	if (int_status & VIRTIO_MMIO_INT_CONFIG) {
		/* Configuration change interrupt */

		/* TODO: Handle configuration changes */
	}

	return OK;
}

/****************************************************************************
 * Name: virtio_blk_read_config
 *
 * Description:
 *   Read block device configuration
 *
 ****************************************************************************/

static void virtio_blk_read_config(virtio_blk_dev_t *dev)
{
	uint32_t i;
	uint32_t *config_ptr = (uint32_t *)&dev->config;
	uint32_t config_size = sizeof(struct virtio_blk_config_s);

	/* Read configuration space in 32-bit chunks */

	for (i = 0; i < (config_size + 3) / 4; i++) {
		config_ptr[i] = virtio_mmio_read32(&dev->mmio_dev, VIRTIO_MMIO_CONFIG + i * 4);
	}
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: virtio_blk_init
 *
 * Description:
 *   Initialize a virtio-blk device
 *
 ****************************************************************************/

int virtio_blk_init(virtio_blk_dev_t *dev, uint32_t device_num)
{
	uint32_t device_features;
	uint32_t device_features_hi;
	uint32_t driver_features = 0;
	uint32_t driver_features_hi = 0;
	uint32_t qnum_max;
	uint16_t queue_num;
	uint8_t status;
	int ret;

	/* Validate input parameters */

	if (!dev) {
		return -EINVAL;
	}

	/* QEMU virt assigns virtio-mmio devices SPI 16..47 (GIC INTID 48..79).
	 * Device N → SPI (16+N) → GIC INTID (48+N).
	 */

	dev->irq = 48 + device_num;
	dev->completion_received = false;

	/* Initialize completion semaphore */

	sem_init(&dev->completion_sem, 0, 0);

	/* Initialize the underlying virtio-mmio device */

	ret = virtio_mmio_init(&dev->mmio_dev, device_num);
	if (ret != OK) {
		return ret;
	}

	/* Check if this is actually a block device */

	if (dev->mmio_dev.device_id != 2) {	/* Block device ID is 2 */
		virtio_mmio_deinit(&dev->mmio_dev);
		return -ENODEV;
	}

	/* Step 1: Acknowledge device */

	virtio_mmio_set_status(&dev->mmio_dev, VIRTIO_CONFIG_STATUS_ACK);

	/* Step 2: Declare driver */

	virtio_mmio_set_status(&dev->mmio_dev,
			       VIRTIO_CONFIG_STATUS_ACK |
			       VIRTIO_CONFIG_STATUS_DRIVER);

	/* Step 3: Read and negotiate features */

	/* Low 32 bits */
	device_features = virtio_mmio_get_device_features(&dev->mmio_dev);
	dev->features = device_features;

	if (device_features & VIRTIO_BLK_F_BLK_SIZE) {
		driver_features |= VIRTIO_BLK_F_BLK_SIZE;
	}
	if (device_features & VIRTIO_BLK_F_FLUSH) {
		driver_features |= VIRTIO_BLK_F_FLUSH;
	}

	ret = virtio_mmio_set_driver_features(&dev->mmio_dev, driver_features);
	if (ret != OK) {
		virtio_mmio_deinit(&dev->mmio_dev);
		return ret;
	}

	/* v2 only: negotiate VIRTIO_F_VERSION_1 (bit 32 = bit 0 of high word).
	 * QEMU MMIO v2 requires this before accepting QUEUE_READY.
	 * In v1 (legacy), there is no DRIVER_FEATURES_SEL — writing to offset
	 * 0x020 again would overwrite the low features we just set.
	 */
	if (dev->mmio_dev.version >= 2) {
		virtio_mmio_write32(&dev->mmio_dev, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 1);
		device_features_hi = virtio_mmio_read32(&dev->mmio_dev, VIRTIO_MMIO_DEVICE_FEATURES);
		driver_features_hi = 1;		/* VIRTIO_F_VERSION_1 is mandatory for v2 */
		virtio_mmio_write32(&dev->mmio_dev, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
		virtio_mmio_write32(&dev->mmio_dev, VIRTIO_MMIO_DRIVER_FEATURES, driver_features_hi);
	}

	lldbg("virtio-blk: version=%u dev_feat_hi=0x%08x drv_feat_hi=0x%08x\n",
	      dev->mmio_dev.version, device_features_hi, driver_features_hi);

	/* Step 4: Confirm features are acceptable */

	virtio_mmio_set_status(&dev->mmio_dev,
			       VIRTIO_CONFIG_STATUS_ACK |
			       VIRTIO_CONFIG_STATUS_DRIVER |
			       VIRTIO_CONFIG_STATUS_FEATURES_OK);

	status = virtio_mmio_get_status(&dev->mmio_dev);
	lldbg("virtio-blk: STATUS after FEATURES_OK write: 0x%02x\n", (uint32_t)status);
	if (!(status & VIRTIO_CONFIG_STATUS_FEATURES_OK)) {
		virtio_mmio_set_status(&dev->mmio_dev, VIRTIO_CONFIG_STATUS_FAILED);
		virtio_mmio_deinit(&dev->mmio_dev);
		return -ENOTSUP;
	}

	/* Step 5: Read configuration and set up virtqueue */

	virtio_blk_read_config(dev);

	/* Clamp queue size to what the device actually supports. QEMU virtio-blk
	 * defaults to 128.  Exceeding QUEUE_NUM_MAX causes the device to reject
	 * QUEUE_READY and ignore all subsequent notifies.
	 */
	virtio_mmio_select_queue(&dev->mmio_dev, 0);
	qnum_max = virtio_mmio_get_queue_num_max(&dev->mmio_dev);
	if (qnum_max == 0) {
		virtio_mmio_deinit(&dev->mmio_dev);
		return -ENODEV;
	}
	queue_num = (qnum_max >= 256) ? 256 : (uint16_t)qnum_max;
	/* virtq_init requires a power-of-2 size */
	while (queue_num & (queue_num - 1)) {
		queue_num &= queue_num - 1;
	}
	lldbg("virtio-blk: QUEUE_NUM_MAX=%u, using queue_num=%u\n", qnum_max, queue_num);

	ret = virtq_init(&dev->vq, queue_num);
	if (ret != OK) {
		virtio_mmio_deinit(&dev->mmio_dev);
		return ret;
	}

	/* Register queue with MMIO device */

	if (dev->mmio_dev.version >= 2) {
		/* v2: separate descriptor, driver, and device ring addresses */
		ret = virtio_mmio_setup_queue(&dev->mmio_dev, 0, queue_num,
					      (uint64_t)(uintptr_t)dev->vq.desc,
					      (uint64_t)(uintptr_t)dev->vq.avail,
					      (uint64_t)(uintptr_t)dev->vq.used);
		if (ret != OK) {
			virtq_deinit(&dev->vq);
			virtio_mmio_deinit(&dev->mmio_dev);
			return ret;
		}

		/* v2: verify the device accepted the queue (QUEUE_READY readback) */
		{
			uint32_t qready = virtio_mmio_read32(&dev->mmio_dev,
							     VIRTIO_MMIO_QUEUE_READY);
			lldbg("virtio-blk: QUEUE_READY readback = %u\n", qready);
			if (!qready) {
				lldbg("ERROR: device rejected queue setup\n");
				virtq_deinit(&dev->vq);
				virtio_mmio_deinit(&dev->mmio_dev);
				return -EIO;
			}
		}
	} else {
		/* v1 (legacy): single page-aligned base address via QUEUE_PFN.
		 * vq->desc must be 4096-aligned (ensured by virtq_alloc_memory).
		 */
		ret = virtio_mmio_setup_queue_v1(&dev->mmio_dev, 0, queue_num,
						 (uintptr_t)dev->vq.desc);
		if (ret != OK) {
			virtq_deinit(&dev->vq);
			virtio_mmio_deinit(&dev->mmio_dev);
			return ret;
		}
		lldbg("virtio-blk: v1 queue PFN=0x%08x\n",
		      (uint32_t)((uintptr_t)dev->vq.desc >> 12));
	}

	/* Step 6: Signal driver is ready */

	virtio_mmio_set_status(&dev->mmio_dev,
			       VIRTIO_CONFIG_STATUS_ACK |
			       VIRTIO_CONFIG_STATUS_DRIVER |
			       VIRTIO_CONFIG_STATUS_FEATURES_OK |
			       VIRTIO_CONFIG_STATUS_DRIVER_OK);

	/* Attach and enable interrupt handler */

	ret = irq_attach(dev->irq, virtio_blk_interrupt, dev);
	if (ret != OK) {
		virtq_deinit(&dev->vq);
		virtio_mmio_deinit(&dev->mmio_dev);
		return ret;
	}

	up_enable_irq(dev->irq);

	dev->ready = true;
	return OK;
}

/****************************************************************************
 * Name: virtio_blk_deinit
 *
 * Description:
 *   Deinitialize a virtio-blk device
 *
 ****************************************************************************/

void virtio_blk_deinit(virtio_blk_dev_t *dev)
{
	if (!dev || !dev->ready) {
		return;
	}

	/* Reset device */

	virtio_mmio_set_status(&dev->mmio_dev, VIRTIO_CONFIG_STATUS_RESET);
	dev->ready = false;

	/* Deinitialize the underlying virtio-mmio device */

	virtio_mmio_deinit(&dev->mmio_dev);
}

/****************************************************************************
 * Name: virtio_blk_read
 *
 * Description:
 *   Read sectors from the block device
 *
 ****************************************************************************/

int virtio_blk_read(virtio_blk_dev_t *dev, uint64_t sector, void *buffer, size_t nsectors)
{
	struct virtq_desc desc[3];
	uint32_t blk_size;
	int ret;

	if (!dev || !dev->ready || !buffer || nsectors == 0) {
		return -EINVAL;
	}

	blk_size = dev->config.blk_size ? dev->config.blk_size : 512;

	/* Use DMA-safe device buffers instead of stack-allocated ones */
	dev->req_hdr.type = VIRTIO_BLK_T_IN;
	dev->req_hdr.reserved = 0;
	dev->req_hdr.sector = sector;
	dev->req_footer.status = 0xff; /* Set to invalid to detect completion */
	dev->req_ndesc = 3;

	/* Memory barrier to ensure DMA buffers are ready */
	__sync_synchronize();

	desc[0].addr  = (uint64_t)(uintptr_t)&dev->req_hdr;
	desc[0].len   = sizeof(dev->req_hdr);
	desc[0].flags = 0;
	desc[0].next  = 1;

	desc[1].addr  = (uint64_t)(uintptr_t)buffer;
	desc[1].len   = nsectors * blk_size;
	desc[1].flags = VIRTQ_DESC_F_WRITE;
	desc[1].next  = 2;

	desc[2].addr  = (uint64_t)(uintptr_t)&dev->req_footer;
	desc[2].len   = sizeof(dev->req_footer);
	desc[2].flags = VIRTQ_DESC_F_WRITE;
	desc[2].next  = 0;

	/* Add the 3-descriptor chain (header → data → footer) to the virtqueue */

	ret = virtq_add_buffer(&dev->vq, desc, 3);
	if (ret != OK) {
		return ret;
	}

	/* Notify the device via the MMIO queue notify register */

	dev->completion_received = false;
	virtq_kick(&dev->vq);
	virtio_mmio_queue_notify(&dev->mmio_dev, 0);

	/* Wait for the interrupt handler to signal completion */

	if (sem_wait(&dev->completion_sem) != OK) {
		return -EIO;
	}

	/* Memory barrier to ensure we read the completed DMA buffers */
	__sync_synchronize();

	if (virtq_get_buffer(&dev->vq, NULL) == -EAGAIN) {
		return -EIO;
	}

	if (dev->req_footer.status != VIRTIO_BLK_S_OK) {
		return -EIO;
	}

	return OK;
}

/****************************************************************************
 * Name: virtio_blk_write
 *
 * Description:
 *   Write sectors to the block device
 *
 ****************************************************************************/

int virtio_blk_write(virtio_blk_dev_t *dev, uint64_t sector, const void *buffer, size_t nsectors)
{
	struct virtq_desc desc[3];
	uint32_t blk_size;
	int ret;

	if (!dev || !dev->ready || !buffer || nsectors == 0) {
		return -EINVAL;
	}

	blk_size = dev->config.blk_size ? dev->config.blk_size : 512;

	/* Use DMA-safe device buffers instead of stack-allocated ones */
	dev->req_hdr.type = VIRTIO_BLK_T_OUT;
	dev->req_hdr.reserved = 0;
	dev->req_hdr.sector = sector;
	dev->req_footer.status = 0xff; /* Set to invalid to detect completion */
	dev->req_ndesc = 3;

	/* Memory barrier to ensure DMA buffers are ready */
	__sync_synchronize();

	desc[0].addr  = (uint64_t)(uintptr_t)&dev->req_hdr;
	desc[0].len   = sizeof(dev->req_hdr);
	desc[0].flags = 0;
	desc[0].next  = 1;

	desc[1].addr  = (uint64_t)(uintptr_t)buffer;
	desc[1].len   = nsectors * blk_size;
	desc[1].flags = 0;
	desc[1].next  = 2;

	desc[2].addr  = (uint64_t)(uintptr_t)&dev->req_footer;
	desc[2].len   = sizeof(dev->req_footer);
	desc[2].flags = VIRTQ_DESC_F_WRITE;
	desc[2].next  = 0;

	/* Add the 3-descriptor chain (header → data → footer) to the virtqueue */

	ret = virtq_add_buffer(&dev->vq, desc, 3);
	if (ret != OK) {
		return ret;
	}

	/* Notify the device via the MMIO queue notify register */

	dev->completion_received = false;
	virtq_kick(&dev->vq);
	virtio_mmio_queue_notify(&dev->mmio_dev, 0);

	/* Wait for the interrupt handler to signal completion */

	if (sem_wait(&dev->completion_sem) != OK) {
		return -EIO;
	}

	/* Memory barrier to ensure we read the completed DMA buffers */
	__sync_synchronize();

	if (virtq_get_buffer(&dev->vq, NULL) == -EAGAIN) {
		return -EIO;
	}

	if (dev->req_footer.status != VIRTIO_BLK_S_OK) {
		return -EIO;
	}

	return OK;
}

/****************************************************************************
 * Name: virtio_blk_flush
 *
 * Description:
 *   Flush pending writes to the block device
 *
 ****************************************************************************/

int virtio_blk_flush(virtio_blk_dev_t *dev)
{
	/* TODO: Implement actual flush operation */
	/* This is a placeholder implementation */

	if (!dev || !dev->ready) {
		return -EINVAL;
	}

	return OK;
}

/****************************************************************************
 * Name: virtio_blk_get_capacity
 *
 * Description:
 *   Get the block device capacity in sectors
 *
 ****************************************************************************/

uint64_t virtio_blk_get_capacity(virtio_blk_dev_t *dev)
{
	if (!dev || !dev->ready) {
		return 0;
	}

	return dev->config.capacity;
}

/****************************************************************************
 * Name: virtio_blk_get_block_size
 *
 * Description:
 *   Get the block device block size
 *
 ****************************************************************************/

uint32_t virtio_blk_get_block_size(virtio_blk_dev_t *dev)
{
	if (!dev || !dev->ready) {
		return 0;
	}

	return dev->config.blk_size;
}
