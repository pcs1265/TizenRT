/****************************************************************************
 *
 * Copyright 2025 Samsung Electronics All Rights Reserved.  
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
/****************************************************************************
 * arch/arm/src/qemu-virt/qemu_s1_boot.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The

 * ASF licenses this file to you under the Apache License, Version 2.0 (
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <stdint.h>
#include <stddef.h>
#include "s1_bootparam.h"
#include "s1_binheader.h"
#include "s1_printf.h"
#include "s1_boot_crc.h"
#include "s1_utils.h"
#include "s1_virtio_blk.h"

#include "sctlr.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/



/****************************************************************************
 * Private Functions
 ****************************************************************************/

#define S1_KERNEL_LOAD_BUF      ((void *)CONFIG_QEMU_KERNEL_LOAD_ADDR)
#define S1_BOOTPARAM_BUF0       ((bpdata_t *)(CONFIG_QEMU_KERNEL_LOAD_ADDR + 0x03000000))
#define S1_BOOTPARAM_BUF1       ((bpdata_t *)(CONFIG_QEMU_KERNEL_LOAD_ADDR + 0x03001000))

static uint32_t s1_get_kernel_offset(const bpdata_t *bp_data, uint8_t index)
{
	const uint8_t *kernel_address =
		(const uint8_t *)bp_data->kernel_address;

	return s1_get_unaligned_le32(kernel_address + (index * sizeof(uint32_t)));
}

static uint32_t s1_get_bin_size(const binheader_t *binheader)
{
	return s1_get_unaligned_le32((const uint8_t *)binheader + 10);
}

static bpdata_t *s1_verify_bootparam(void)
{
	bpdata_t *bp_datas[2];
	bool bp_valids[2] = {false, false};
	uint32_t calculated_crc;
	uint32_t bp_offsets[2];

	bp_datas[0] = S1_BOOTPARAM_BUF0;
	bp_datas[1] = S1_BOOTPARAM_BUF1;
	bp_offsets[0] = CONFIG_FLASH_SIZE - BOOTPARAM_PART_SIZE;
	bp_offsets[1] = CONFIG_FLASH_SIZE - (BOOTPARAM_PART_SIZE / 2);

	for (int i = 0; i < 2; i++) {
		if (s1_virtio_blk_read(bp_offsets[i], bp_datas[i], BOOTPARAM_PART_SIZE / 2) != 0) {
			s1_boot_printf("BP%d read failed from block device offset 0x%x err %d\n",
				i, bp_offsets[i], s1_virtio_blk_get_last_error());
			bp_valids[i] = false;
			continue;
		}

		calculated_crc = calculate_bootparam_crc(bp_datas[i]);
		if (calculated_crc != bp_datas[i]->crc_hash) {
			s1_boot_printf("BP%d Invalid. CRC Expected: 0x%x Got: 0x%x\n", i, bp_datas[i]->crc_hash, calculated_crc);
			bp_valids[i] = false;
		} else {
			s1_boot_printf("BP%d Valid. Version: %d\n", i, bp_datas[i]->version);
			bp_valids[i] = true;
		}
	}

	if (bp_valids[0] && bp_valids[1]) {
		if (bp_datas[0]->version >= bp_datas[1]->version) {
			s1_boot_printf("Use BP0.\n");
			return bp_datas[0];
		} else {
			s1_boot_printf("Use BP1.\n");
			return bp_datas[1];
		}
	} else if (bp_valids[0]) {
		s1_boot_printf("Use BP0.\n");
		return bp_datas[0];
	} else if (bp_valids[1]) {
		s1_boot_printf("Use BP1.\n");
		return bp_datas[1];
	} else {
		s1_boot_printf("No valid bootparam found.\n");
		return NULL;
	}
}

static binheader_t *s1_load_kernel(uint32_t kernelpart_offset)
{
	binheader_t *kernel_binheader;
	uint32_t calculated_crc;
	uint32_t image_size;
	uint32_t read_size;

	kernel_binheader = (binheader_t *)S1_KERNEL_LOAD_BUF;

	if (s1_virtio_blk_read(kernelpart_offset, kernel_binheader, S1_VIRTIO_BLK_SECTOR_SIZE) != 0) {
		s1_boot_printf("Kernel header read failed from block device offset 0x%x\n", kernelpart_offset);
		return NULL;
	}

	image_size = sizeof(binheader_t) + s1_get_bin_size(kernel_binheader);
	read_size = (image_size + S1_VIRTIO_BLK_SECTOR_SIZE - 1) &
		~(S1_VIRTIO_BLK_SECTOR_SIZE - 1);

	if (s1_virtio_blk_read(kernelpart_offset, kernel_binheader, read_size) != 0) {
		s1_boot_printf("Kernel image read failed from block device offset 0x%x\n", kernelpart_offset);
		return NULL;
	}

	s1_boot_printf("Kernel Partition CRC Check... ");
	calculated_crc = calculate_kernel_crc(kernel_binheader);

	if (calculated_crc != kernel_binheader->crc_hash)
	{
		printf("FAILED!\n");
		s1_boot_printf("CRC Expected: 0x%x Got: 0x%x\n", kernel_binheader->crc_hash, calculated_crc);
		return NULL;
	}
	else
	{
		printf("PASSED.\n");
		return kernel_binheader;
	}
}

static void s1_prepare_environment(void)
{
	extern uint32_t __ram_start, __stack_top;

	s1_boot_printf("Clearing RAM for kernel...");
	void *ram_start_to_clear = (void *)&__stack_top;
	size_t ram_size_to_clear = ((uint32_t)&__ram_start + CONFIG_RAM_SIZE) - (uint32_t)&__stack_top;
	s1_memset(ram_start_to_clear, 0, ram_size_to_clear);
	printf(" DONE.\n");
}

static void s1_boot_kernel(binheader_t *kernel_binheader)
{
	uint32_t boot_addr;
	void (*kernel_entry)(void);

#ifdef CONFIG_BOOT_RUNFROMSDRAM
	s1_boot_printf("Preparing kernel in RAM(%d Bytes)... ", s1_get_bin_size(kernel_binheader));
	void *dst = S1_KERNEL_LOAD_BUF;
	const void *src = (const void *)((uintptr_t)kernel_binheader + sizeof(binheader_t));
	s1_memcpy(dst, src, s1_get_bin_size(kernel_binheader));
	printf("DONE.\n");

	boot_addr = CONFIG_QEMU_KERNEL_LOAD_ADDR;
#else
#error "S1 virtio-blk boot requires CONFIG_BOOT_RUNFROMSDRAM"
#endif

	s1_boot_printf("Jumping to kernel at 0x%x\n", boot_addr);
	

	kernel_entry = (void (*)(void))boot_addr;
	kernel_entry();
}


/****************************************************************************
 * Public Functions
 ****************************************************************************/

void s1_boot(void)
{
	bpdata_t *bp_data;
	binheader_t *kernel_binheader;
	uint32_t kernelpart_offset;

	cp15_wrvbar(CONFIG_FLASH_START_ADDR);

	s1_boot_printf("Bootloader Started.\n");
	s1_prepare_environment();

	if (s1_virtio_blk_init() != 0) {
		s1_boot_printf("virtio-blk init failed.\n");
		goto err;
	}

	bp_data = s1_verify_bootparam();
	if (!bp_data)
	{
		goto err;
	}

	s1_boot_printf("Active Index : 0x%x\n", bp_data->active_idx);
	if (bp_data->active_idx >= BOOTPARAM_COUNT) {
		s1_boot_printf("Invalid active index: 0x%x\n", bp_data->active_idx);
		goto err;
	}

	kernelpart_offset = s1_get_kernel_offset(bp_data, bp_data->active_idx);
	s1_boot_printf("Kernel Partition Offset : 0x%x\n", kernelpart_offset);

	kernel_binheader = s1_load_kernel(kernelpart_offset);
	if (!kernel_binheader)
	{
		goto err;
	}

	s1_boot_kernel(kernel_binheader);

err:
	for (;;){};
}
