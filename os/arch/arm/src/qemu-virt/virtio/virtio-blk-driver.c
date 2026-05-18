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
#include <tinyara/fs/fs.h>
#ifdef CONFIG_QEMU_VIRT_VIRTIO_BLK_MTD
#include <tinyara/fs/ioctl.h>
#include <tinyara/fs/mtd.h>
#endif
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <debug.h>

#include "virtio-blk.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VIRTIO_BLK_DEVPATH	"/dev/virtblk0"
#define VIRTIO_BLK_CHARPATH	"/dev/virtblkc0"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static virtio_blk_dev_t g_virtio_blk_dev;
static bool g_virtio_blk_ready;

#ifdef CONFIG_QEMU_VIRT_VIRTIO_BLK_MTD
static struct mtd_dev_s g_virtio_blk_mtd;
static uint8_t g_virtio_blk_sector[512];
static uint8_t g_virtio_blk_erase_sector[512];
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int     vblk_open(FAR struct inode *inode);
static int     vblk_close(FAR struct inode *inode);
static ssize_t vblk_read(FAR struct inode *inode, FAR unsigned char *buf,
			 size_t start_sector, unsigned int nsectors);
static ssize_t vblk_write(FAR struct inode *inode, FAR const unsigned char *buf,
			  size_t start_sector, unsigned int nsectors);
static int     vblk_geometry(FAR struct inode *inode, FAR struct geometry *geo);
static int     vblk_ioctl(FAR struct inode *inode, int cmd, unsigned long arg);

#ifdef CONFIG_QEMU_VIRT_VIRTIO_BLK_MTD
static int     vblk_mtd_erase(FAR struct mtd_dev_s *dev, off_t startblock,
			      size_t nblocks);
static ssize_t vblk_mtd_bread(FAR struct mtd_dev_s *dev, off_t startblock,
			      size_t nblocks, FAR uint8_t *buffer);
static ssize_t vblk_mtd_bwrite(FAR struct mtd_dev_s *dev, off_t startblock,
			       size_t nblocks, FAR const uint8_t *buffer);
static ssize_t vblk_mtd_read(FAR struct mtd_dev_s *dev, off_t offset,
			     size_t nbytes, FAR uint8_t *buffer);
#ifdef CONFIG_MTD_BYTE_WRITE
static ssize_t vblk_mtd_write(FAR struct mtd_dev_s *dev, off_t offset,
			      size_t nbytes, FAR const uint8_t *buffer);
#endif
static int     vblk_mtd_ioctl(FAR struct mtd_dev_s *dev, int cmd,
			      unsigned long arg);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct block_operations g_vblk_bops = {
	vblk_open,	/* open     */
	vblk_close,	/* close    */
	vblk_read,	/* read     */
	vblk_write,	/* write    */
	vblk_geometry,	/* geometry */
	vblk_ioctl,	/* ioctl    */
	NULL,		/* unlink   */
};

static int virtio_blk_ensure_initialized(uint32_t device_num)
{
	int ret;

	if (g_virtio_blk_ready) {
		return OK;
	}

	ret = virtio_blk_init(&g_virtio_blk_dev, device_num);
	if (ret != OK) {
		lldbg("ERROR: virtio_blk_init failed: %d\n", ret);
		return ret;
	}

	g_virtio_blk_ready = true;

#ifdef CONFIG_QEMU_VIRT_VIRTIO_BLK_MTD
	memset(g_virtio_blk_erase_sector, 0xff, sizeof(g_virtio_blk_erase_sector));
#endif

	return OK;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int vblk_open(FAR struct inode *inode)
{
	return OK;
}

static int vblk_close(FAR struct inode *inode)
{
	return OK;
}

static ssize_t vblk_read(FAR struct inode *inode, FAR unsigned char *buf,
			 size_t start_sector, unsigned int nsectors)
{
	int ret;

	ret = virtio_blk_read(&g_virtio_blk_dev, (uint64_t)start_sector, buf, nsectors);
	if (ret != OK) {
		return ret;
	}

	return (ssize_t)nsectors;
}

static ssize_t vblk_write(FAR struct inode *inode, FAR const unsigned char *buf,
			  size_t start_sector, unsigned int nsectors)
{
	int ret;

	ret = virtio_blk_write(&g_virtio_blk_dev, (uint64_t)start_sector, buf, nsectors);
	if (ret != OK) {
		return ret;
	}

	return (ssize_t)nsectors;
}

static int vblk_geometry(FAR struct inode *inode, FAR struct geometry *geo)
{
	if (!geo) {
		return -EINVAL;
	}

	geo->geo_available    = true;
	geo->geo_mediachanged = false;
	geo->geo_writeenabled = true;
	geo->geo_nsectors     = (size_t)virtio_blk_get_capacity(&g_virtio_blk_dev);
	geo->geo_sectorsize   = (size_t)virtio_blk_get_block_size(&g_virtio_blk_dev);

	if (geo->geo_sectorsize == 0) {
		geo->geo_sectorsize = 512;
	}

	return OK;
}

static int vblk_ioctl(FAR struct inode *inode, int cmd, unsigned long arg)
{
	return -ENOTTY;
}

#ifdef CONFIG_QEMU_VIRT_VIRTIO_BLK_MTD
static int vblk_mtd_erase(FAR struct mtd_dev_s *dev, off_t startblock,
			  size_t nblocks)
{
	uint64_t sector = (uint64_t)startblock * 8;
	size_t total = nblocks * 8;

	while (total-- > 0) {
		int ret = virtio_blk_write(&g_virtio_blk_dev, sector,
					   g_virtio_blk_erase_sector, 1);
		if (ret != OK) {
			return ret;
		}

		sector++;
	}

	return OK;
}

static ssize_t vblk_mtd_bread(FAR struct mtd_dev_s *dev, off_t startblock,
			      size_t nblocks, FAR uint8_t *buffer)
{
	int ret;

	ret = virtio_blk_read(&g_virtio_blk_dev, (uint64_t)startblock,
			      buffer, nblocks);
	if (ret != OK) {
		return ret;
	}

	return nblocks;
}

static ssize_t vblk_mtd_bwrite(FAR struct mtd_dev_s *dev, off_t startblock,
			       size_t nblocks, FAR const uint8_t *buffer)
{
	int ret;

	ret = virtio_blk_write(&g_virtio_blk_dev, (uint64_t)startblock,
			       buffer, nblocks);
	if (ret != OK) {
		return ret;
	}

	return nblocks;
}

static ssize_t vblk_mtd_read(FAR struct mtd_dev_s *dev, off_t offset,
			     size_t nbytes, FAR uint8_t *buffer)
{
	size_t copied = 0;

	while (copied < nbytes) {
		uint64_t sector = (uint64_t)(offset + copied) / 512;
		size_t sector_offset = (offset + copied) % 512;
		size_t copy_len = 512 - sector_offset;
		int ret;

		if (copy_len > nbytes - copied) {
			copy_len = nbytes - copied;
		}

		ret = virtio_blk_read(&g_virtio_blk_dev, sector,
				      g_virtio_blk_sector, 1);
		if (ret != OK) {
			return ret;
		}

		memcpy(buffer + copied, g_virtio_blk_sector + sector_offset,
		       copy_len);
		copied += copy_len;
	}

	return nbytes;
}

#ifdef CONFIG_MTD_BYTE_WRITE
static ssize_t vblk_mtd_write(FAR struct mtd_dev_s *dev, off_t offset,
			      size_t nbytes, FAR const uint8_t *buffer)
{
	size_t copied = 0;

	while (copied < nbytes) {
		uint64_t sector = (uint64_t)(offset + copied) / 512;
		size_t sector_offset = (offset + copied) % 512;
		size_t copy_len = 512 - sector_offset;
		int ret;

		if (copy_len > nbytes - copied) {
			copy_len = nbytes - copied;
		}

		ret = virtio_blk_read(&g_virtio_blk_dev, sector,
				      g_virtio_blk_sector, 1);
		if (ret != OK) {
			return ret;
		}

		memcpy(g_virtio_blk_sector + sector_offset, buffer + copied,
		       copy_len);

		ret = virtio_blk_write(&g_virtio_blk_dev, sector,
				       g_virtio_blk_sector, 1);
		if (ret != OK) {
			return ret;
		}

		copied += copy_len;
	}

	return nbytes;
}
#endif

static int vblk_mtd_ioctl(FAR struct mtd_dev_s *dev, int cmd,
			  unsigned long arg)
{
	switch (cmd) {
	case MTDIOC_GEOMETRY:
		{
			FAR struct mtd_geometry_s *geo =
				(FAR struct mtd_geometry_s *)arg;
			uint64_t capacity;

			if (!geo) {
				return -EINVAL;
			}

			capacity = virtio_blk_get_capacity(&g_virtio_blk_dev);
			memset(geo, 0, sizeof(*geo));
			geo->blocksize = 512;
			geo->erasesize = 4096;
			geo->neraseblocks = (uint32_t)(capacity / 8);
		}
		return OK;

	case MTDIOC_BULKERASE:
		{
			uint64_t capacity = virtio_blk_get_capacity(&g_virtio_blk_dev);
			return vblk_mtd_erase(dev, 0, (size_t)(capacity / 8));
		}

	case MTDIOC_ERASESTATE:
		{
			FAR uint8_t *result = (FAR uint8_t *)arg;
			if (!result) {
				return -EINVAL;
			}

			*result = 0xff;
		}
		return OK;

	default:
		return -ENOTTY;
	}
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: virtio_blk_driver_initialize
 *
 * Description:
 *   Initialize virtio-blk and register it as a block device + character
 *   device for user-space access.
 *
 *   Block device:     /dev/virtblk0   (kernel-facing)
 *   Character device: /dev/virtblkc0  (user-space read/write/lseek)
 *
 ****************************************************************************/

int virtio_blk_driver_initialize(uint32_t device_num)
{
	int ret;

	ret = virtio_blk_ensure_initialized(device_num);
	if (ret != OK) {
		return ret;
	}

	ret = register_blockdriver(VIRTIO_BLK_DEVPATH, &g_vblk_bops, 0666, NULL);
	if (ret != OK) {
		lldbg("ERROR: register_blockdriver failed: %d\n", ret);
		virtio_blk_deinit(&g_virtio_blk_dev);
		return ret;
	}

	ret = bchdev_register(VIRTIO_BLK_DEVPATH, VIRTIO_BLK_CHARPATH, false);
	if (ret != OK) {
		lldbg("ERROR: bchdev_register failed: %d\n", ret);
		unregister_blockdriver(VIRTIO_BLK_DEVPATH);
		virtio_blk_deinit(&g_virtio_blk_dev);
		return ret;
	}

	lldbg("virtio-blk registered: %s -> %s\n", VIRTIO_BLK_DEVPATH, VIRTIO_BLK_CHARPATH);
	return OK;
}

#ifdef CONFIG_QEMU_VIRT_VIRTIO_BLK_MTD
FAR struct mtd_dev_s *virtio_blk_mtd_initialize(uint32_t device_num)
{
	int ret;

	ret = virtio_blk_ensure_initialized(device_num);
	if (ret != OK) {
		return NULL;
	}

	memset(&g_virtio_blk_mtd, 0, sizeof(g_virtio_blk_mtd));
	g_virtio_blk_mtd.erase = vblk_mtd_erase;
	g_virtio_blk_mtd.bread = vblk_mtd_bread;
	g_virtio_blk_mtd.bwrite = vblk_mtd_bwrite;
	g_virtio_blk_mtd.read = vblk_mtd_read;
#ifdef CONFIG_MTD_BYTE_WRITE
	g_virtio_blk_mtd.write = vblk_mtd_write;
#endif
	g_virtio_blk_mtd.ioctl = vblk_mtd_ioctl;
#ifdef CONFIG_MTD_REGISTRATION
	g_virtio_blk_mtd.name = "virtio-blk-mtd";
#endif

	return &g_virtio_blk_mtd;
}
#endif
