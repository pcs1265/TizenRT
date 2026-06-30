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
 * os/board/qemu-virt/src/qemu_boardinit.c
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
 * WARRANTIES, OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/arch.h> 
#include <tinyara/config.h> 
#include <stdint.h> 
#include <stdio.h>
#include <tinyara/board.h> 
#include <tinyara/kthread.h>
#include <tinyara/fs/mtd.h> 
#include <tinyara/netmgr/netdev_mgr.h>
#include <unistd.h>
#include "common.h"

#include "qemu_cfi.h"
#include "qemu_mtd_cfi.h"

#if defined(CONFIG_QEMU_VIRT_VIRTIO_BLK) || defined(CONFIG_QEMU_VIRT_VIRTIO_BLK_MTD)
#include "virtio/virtio-blk.h"
#endif

#if defined(CONFIG_QEMU_VIRT_VIRTIO_NET) && defined(CONFIG_NET_NETMGR) && defined(CONFIG_LWIP_DHCPC)
extern int _netdev_dhcpc_start(const char *intf);
extern struct netdev *nm_get_netdev(uint8_t *ifname);
extern int nm_ifup(struct netdev *dev);
#endif

#ifdef CONFIG_QEMU_VIRT_VIRTIO_SERIAL
#include "virtio/virtio-serial.h"
#endif

#ifdef CONFIG_QEMU_VIRT_VIRTIO_SERIAL_CHAR
#include "virtio/virtio-serial-char.h"
#endif


/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_QEMU_VIRT_VIRTIO_NET) && defined(CONFIG_NET_NETMGR) && defined(CONFIG_LWIP_DHCPC)
#define QEMU_AUTO_NETIF "eth0"
#define QEMU_AUTO_NET_RETRIES 5
#define QEMU_AUTO_NET_PRIORITY 100
#define QEMU_AUTO_NET_STACKSIZE 2048
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_QEMU_VIRT_VIRTIO_NET) && defined(CONFIG_NET_NETMGR) && defined(CONFIG_LWIP_DHCPC)
static int qemu_auto_netinit(int argc, char *argv[])
{
	struct netdev *dev;
	int dhcp_ret = ERROR;
	int ifup_ret = ERROR;
	int retry;

	(void)argc;
	(void)argv;

	/* board_initialize() runs before net_initialize(); let the net stack start. */
	sleep(1);

	for (retry = 0; retry < QEMU_AUTO_NET_RETRIES; retry++) {
		dhcp_ret = _netdev_dhcpc_start(QEMU_AUTO_NETIF);
		if (dhcp_ret == OK) {
			break;
		}

		sleep(1);
	}

	if (dhcp_ret != OK) {
		printf("qemu-net: DHCP failed on %s: %d\n", QEMU_AUTO_NETIF, dhcp_ret);
		return dhcp_ret;
	}

	dev = nm_get_netdev((uint8_t *)QEMU_AUTO_NETIF);
	if (dev != NULL) {
		ifup_ret = nm_ifup(dev);
	}

	printf("qemu-net: %s DHCP %s, ifup %s\n",
	       QEMU_AUTO_NETIF,
	       (dhcp_ret == OK) ? "OK" : "failed",
	       (ifup_ret == OK) ? "OK" : "failed");

	return ifup_ret;
}

static void qemu_start_auto_netinit(void)
{
	int ret;

	ret = kernel_thread("qemu_netinit",
	                    QEMU_AUTO_NET_PRIORITY,
	                    QEMU_AUTO_NET_STACKSIZE,
	                    qemu_auto_netinit,
	                    (char * const *)NULL);
	if (ret < 0) {
		lldbg("ERROR: qemu_netinit thread failed: %d\n", ret);
	}
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/



void qemu_mount_partitions(void)
{
	int ret;
	struct mtd_dev_s *mtd;
	partition_info_t partinfo;

#ifdef CONFIG_QEMU_VIRT_VIRTIO_BLK_MTD
	mtd = virtio_blk_mtd_initialize(0);
#else
	mtd = cfi_initialize(CONFIG_FLASH_START_ADDR, CONFIG_FLASH_START_ADDR + CONFIG_FLASH_SIZE, 0x04);
#endif
	if (!mtd)
	{
		lldbg("ERROR: mtd_initialize failed\n");
		return;
	}

	/* Configure mtd partitions */
	ret = configure_mtd_partitions(mtd, 0, &partinfo);
	if (ret != OK) {
		lldbg("ERROR: configure_mtd_partitions for primary flash failed\n");
		return;
	}
	
#ifdef CONFIG_AUTOMOUNT
	automount_fs_partition(&partinfo);
#endif

#ifdef CONFIG_RESOURCE_FS
	if (binary_manager_mount_resource() != OK) {
		lldbg("ERROR: Failed to mount resource\n");
	}
#endif
}

/****************************************************************************
 * Name: board_initialize
 *
 * Description:
 *   If CONFIG_BOARD_INITIALIZE is selected, then an additional
 *   initialization call will be performed in the boot-up sequence to a
 *   function called board_initialize().  board_initialize() will be
 *   called immediately after up_initialize() is called and just before the
 *   initial application is started.  This additional initialization phase
 *   may be used, for example, to initialize board-specific device drivers.
 *
 ****************************************************************************/

#ifdef CONFIG_BOARD_INITIALIZE

void board_initialize(void)
{

	/* init console */
#ifndef CONFIG_PLATFORM_TIZENRT_OS
	shell_init_rom(0, 0);
#endif
	qemu_mount_partitions();

#ifdef CONFIG_FTL_ENABLED
	app_ftl_init();
#endif

#ifdef CONFIG_QEMU_VIRT_VIRTIO_BLK
	virtio_blk_driver_initialize(0);
#endif

#ifdef CONFIG_QEMU_VIRT_VIRTIO_SERIAL
	virtio_serial_driver_initialize(CONFIG_QEMU_VIRT_VIRTIO_SERIAL_DEVICE_NUM);
#endif

#ifdef CONFIG_QEMU_VIRT_VIRTIO_SERIAL_CHAR
	virtio_serial_char_driver_initialize(CONFIG_QEMU_VIRT_VIRTIO_SERIAL_CHAR_DEVICE_NUM);
#endif

#if defined(CONFIG_QEMU_VIRT_VIRTIO_NET) && defined(CONFIG_NET_NETMGR) && defined(CONFIG_LWIP_DHCPC)
	qemu_start_auto_netinit();
#endif
}
#else
#error "CONFIG_BOARD_INITIALIZE MUST ENABLE"
#endif

int board_app_initialize(void)
{
	return OK;
}
