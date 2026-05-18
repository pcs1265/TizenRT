/****************************************************************************
 *
 * Copyright 2026 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

#include <tinyara/config.h>
#include <stdint.h>
#include <stddef.h>

#include "s1_virtio_blk.h"
#include "s1_utils.h"

#define VIRTIO_MMIO_BASE_ADDR             0x0a000000
#define VIRTIO_MMIO_DEVICE_SPACING        0x200
#define VIRTIO_MMIO_MAGIC_VALUE           0x000
#define VIRTIO_MMIO_VERSION               0x004
#define VIRTIO_MMIO_DEVICE_ID             0x008
#define VIRTIO_MMIO_DEVICE_FEATURES       0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL   0x014
#define VIRTIO_MMIO_DRIVER_FEATURES       0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL   0x024
#define VIRTIO_MMIO_GUEST_PAGE_SIZE       0x028
#define VIRTIO_MMIO_QUEUE_SEL             0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX         0x034
#define VIRTIO_MMIO_QUEUE_NUM             0x038
#define VIRTIO_MMIO_QUEUE_ALIGN           0x03c
#define VIRTIO_MMIO_QUEUE_PFN             0x040
#define VIRTIO_MMIO_QUEUE_READY           0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY          0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS      0x060
#define VIRTIO_MMIO_INTERRUPT_ACK         0x064
#define VIRTIO_MMIO_STATUS                0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW        0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH       0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW       0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH      0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW        0x0a0
#define VIRTIO_MMIO_QUEUE_USED_HIGH       0x0a4

#define VIRTIO_STATUS_ACK                 0x01
#define VIRTIO_STATUS_DRIVER              0x02
#define VIRTIO_STATUS_DRIVER_OK           0x04
#define VIRTIO_STATUS_FEATURES_OK         0x08
#define VIRTIO_STATUS_FAILED              0x80

#define VIRTIO_DEVICE_ID_BLOCK            2
#define VIRTIO_MAGIC                      0x74726976
#define VIRTIO_F_VERSION_1                1

#define VIRTQ_DESC_F_NEXT                 1
#define VIRTQ_DESC_F_WRITE                2
#define VIRTQ_AVAIL_F_NO_INTERRUPT        1

#define VIRTIO_BLK_T_IN                   0
#define VIRTIO_BLK_S_OK                   0

#define S1_VIRTIO_DEVICE_NUM              0
#define S1_VIRTIO_MMIO_BASE               \
	(VIRTIO_MMIO_BASE_ADDR + (S1_VIRTIO_DEVICE_NUM * VIRTIO_MMIO_DEVICE_SPACING))

#define S1_VIRTQ_NUM                      8
#define S1_VIRTQ_BASE                     0x43000000
#define S1_VIRTQ_DESC_ADDR                S1_VIRTQ_BASE
#define S1_VIRTQ_AVAIL_ADDR               (S1_VIRTQ_BASE + 0x80)
#define S1_VIRTQ_USED_ADDR                (S1_VIRTQ_BASE + 0x1000)

#define S1_VIRTIO_READ_TIMEOUT            0x01000000

struct s1_virtq_desc_s {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next;
} __attribute__((packed));

struct s1_virtq_avail_s {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[S1_VIRTQ_NUM];
} __attribute__((packed));

struct s1_virtq_used_elem_s {
	uint32_t id;
	uint32_t len;
} __attribute__((packed));

struct s1_virtq_used_s {
	uint16_t flags;
	volatile uint16_t idx;
	struct s1_virtq_used_elem_s ring[S1_VIRTQ_NUM];
} __attribute__((packed));

struct s1_virtio_blk_req_hdr_s {
	uint32_t type;
	uint32_t reserved;
	uint64_t sector;
} __attribute__((packed));

struct s1_virtio_blk_req_footer_s {
	volatile uint8_t status;
} __attribute__((packed));

#define S1_VIRTQ_DESC  ((volatile struct s1_virtq_desc_s *)S1_VIRTQ_DESC_ADDR)
#define S1_VIRTQ_AVAIL ((volatile struct s1_virtq_avail_s *)S1_VIRTQ_AVAIL_ADDR)
#define S1_VIRTQ_USED  ((volatile struct s1_virtq_used_s *)S1_VIRTQ_USED_ADDR)

static uint16_t g_last_used_idx;
static uint16_t g_avail_idx;
static int g_last_error;

static uint32_t s1_getreg32(uint32_t offset)
{
	return *(volatile uint32_t *)(S1_VIRTIO_MMIO_BASE + offset);
}

static void s1_putreg32(uint32_t offset, uint32_t value)
{
	*(volatile uint32_t *)(S1_VIRTIO_MMIO_BASE + offset) = value;
}

static void s1_dmb(void)
{
	__asm__ volatile ("dmb sy" : : : "memory");
}

static void s1_dsb(void)
{
	__asm__ volatile ("dsb sy" : : : "memory");
}

int s1_virtio_blk_init(void)
{
	uint32_t version;
	uint32_t qnum_max;
	uint32_t status;

	if (s1_getreg32(VIRTIO_MMIO_MAGIC_VALUE) != VIRTIO_MAGIC) {
		g_last_error = -1;
		return -1;
	}

	version = s1_getreg32(VIRTIO_MMIO_VERSION);
	if (s1_getreg32(VIRTIO_MMIO_DEVICE_ID) != VIRTIO_DEVICE_ID_BLOCK) {
		g_last_error = -2;
		return -2;
	}

	s1_putreg32(VIRTIO_MMIO_STATUS, 0);
	s1_putreg32(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACK);
	s1_putreg32(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER);

	s1_putreg32(VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
	s1_putreg32(VIRTIO_MMIO_DRIVER_FEATURES, 0);

	if (version >= 2) {
		s1_putreg32(VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
		s1_putreg32(VIRTIO_MMIO_DRIVER_FEATURES, VIRTIO_F_VERSION_1);
	}

	status = VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK;
	s1_putreg32(VIRTIO_MMIO_STATUS, status);
	if ((s1_getreg32(VIRTIO_MMIO_STATUS) & VIRTIO_STATUS_FEATURES_OK) == 0) {
		s1_putreg32(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
		g_last_error = -3;
		return -3;
	}

	s1_putreg32(VIRTIO_MMIO_QUEUE_SEL, 0);
	qnum_max = s1_getreg32(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (qnum_max < S1_VIRTQ_NUM) {
		s1_putreg32(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
		g_last_error = -4;
		return -4;
	}

	s1_memset((void *)S1_VIRTQ_BASE, 0, 0x2000);
	S1_VIRTQ_AVAIL->flags = VIRTQ_AVAIL_F_NO_INTERRUPT;
	S1_VIRTQ_AVAIL->idx = 0;
	S1_VIRTQ_USED->idx = 0;
	g_last_used_idx = 0;
	g_avail_idx = 0;

	s1_putreg32(VIRTIO_MMIO_QUEUE_NUM, S1_VIRTQ_NUM);
	if (version >= 2) {
		s1_putreg32(VIRTIO_MMIO_QUEUE_DESC_LOW, S1_VIRTQ_DESC_ADDR);
		s1_putreg32(VIRTIO_MMIO_QUEUE_DESC_HIGH, 0);
		s1_putreg32(VIRTIO_MMIO_QUEUE_AVAIL_LOW, S1_VIRTQ_AVAIL_ADDR);
		s1_putreg32(VIRTIO_MMIO_QUEUE_AVAIL_HIGH, 0);
		s1_putreg32(VIRTIO_MMIO_QUEUE_USED_LOW, S1_VIRTQ_USED_ADDR);
		s1_putreg32(VIRTIO_MMIO_QUEUE_USED_HIGH, 0);
		s1_putreg32(VIRTIO_MMIO_QUEUE_READY, 1);
	} else {
		s1_putreg32(VIRTIO_MMIO_GUEST_PAGE_SIZE, 4096);
		s1_putreg32(VIRTIO_MMIO_QUEUE_ALIGN, 4096);
		s1_putreg32(VIRTIO_MMIO_QUEUE_PFN, S1_VIRTQ_BASE >> 12);
	}

	s1_putreg32(VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_DRIVER_OK);
	g_last_error = 0;
	return 0;
}

static int s1_virtio_blk_read_sectors(uint64_t sector, void *buffer,
				      uint32_t nsectors)
{
	volatile struct s1_virtio_blk_req_hdr_s *hdr;
	volatile struct s1_virtio_blk_req_footer_s *footer;
	uint32_t timeout;
	uint16_t slot;

	hdr = (volatile struct s1_virtio_blk_req_hdr_s *)(S1_VIRTQ_BASE + 0x1800);
	footer = (volatile struct s1_virtio_blk_req_footer_s *)(S1_VIRTQ_BASE + 0x1820);

	hdr->type = VIRTIO_BLK_T_IN;
	hdr->reserved = 0;
	hdr->sector = sector;
	footer->status = 0xff;

	S1_VIRTQ_DESC[0].addr = (uint32_t)hdr;
	S1_VIRTQ_DESC[0].len = sizeof(*hdr);
	S1_VIRTQ_DESC[0].flags = VIRTQ_DESC_F_NEXT;
	S1_VIRTQ_DESC[0].next = 1;

	S1_VIRTQ_DESC[1].addr = (uint32_t)buffer;
	S1_VIRTQ_DESC[1].len = nsectors * S1_VIRTIO_BLK_SECTOR_SIZE;
	S1_VIRTQ_DESC[1].flags = VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE;
	S1_VIRTQ_DESC[1].next = 2;

	S1_VIRTQ_DESC[2].addr = (uint32_t)footer;
	S1_VIRTQ_DESC[2].len = sizeof(*footer);
	S1_VIRTQ_DESC[2].flags = VIRTQ_DESC_F_WRITE;
	S1_VIRTQ_DESC[2].next = 0;

	slot = g_avail_idx & (S1_VIRTQ_NUM - 1);
	S1_VIRTQ_AVAIL->ring[slot] = 0;
	s1_dmb();
	g_avail_idx++;
	S1_VIRTQ_AVAIL->idx = g_avail_idx;
	s1_dsb();

	s1_putreg32(VIRTIO_MMIO_QUEUE_NOTIFY, 0);

	timeout = S1_VIRTIO_READ_TIMEOUT;
	while (S1_VIRTQ_USED->idx == g_last_used_idx && timeout-- > 0) {
	}

	if (S1_VIRTQ_USED->idx == g_last_used_idx) {
		g_last_error = -10;
		return -1;
	}

	g_last_used_idx++;
	s1_putreg32(VIRTIO_MMIO_INTERRUPT_ACK, s1_getreg32(VIRTIO_MMIO_INTERRUPT_STATUS));

	if (footer->status != VIRTIO_BLK_S_OK) {
		g_last_error = -20 - footer->status;
		return -2;
	}

	g_last_error = 0;
	return 0;
}

int s1_virtio_blk_read(uint32_t byte_offset, void *buffer, size_t nbytes)
{
	uint8_t *dst = (uint8_t *)buffer;
	uint64_t sector = byte_offset / S1_VIRTIO_BLK_SECTOR_SIZE;
	uint32_t nsectors;

	if ((byte_offset % S1_VIRTIO_BLK_SECTOR_SIZE) != 0 ||
	    (((uintptr_t)buffer % S1_VIRTIO_BLK_SECTOR_SIZE) != 0) ||
	    (nbytes % S1_VIRTIO_BLK_SECTOR_SIZE) != 0) {
		g_last_error = -30;
		return -1;
	}

	while (nbytes > 0) {
		nsectors = nbytes / S1_VIRTIO_BLK_SECTOR_SIZE;
		if (nsectors > 128) {
			nsectors = 128;
		}

		if (s1_virtio_blk_read_sectors(sector, dst, nsectors) != 0) {
			return -2;
		}

		sector += nsectors;
		dst += nsectors * S1_VIRTIO_BLK_SECTOR_SIZE;
		nbytes -= nsectors * S1_VIRTIO_BLK_SECTOR_SIZE;
	}

	return 0;
}

int s1_virtio_blk_get_last_error(void)
{
	return g_last_error;
}
