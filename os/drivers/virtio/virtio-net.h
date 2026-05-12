/****************************************************************************
 *
 * Copyright 2026 Samsung Electronics All Rights Reserved.
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

#ifndef __DRIVERS_VIRTIO_VIRTIO_NET_H
#define __DRIVERS_VIRTIO_VIRTIO_NET_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <stdint.h>
#include <stdbool.h>

#include "virtio-mmio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VIRTIO_NET_F_CSUM		(1 << 0)
#define VIRTIO_NET_F_MAC		(1 << 5)
#define VIRTIO_NET_F_STATUS		(1 << 16)
#define VIRTIO_NET_F_MRG_RXBUF		(1 << 15)

#define VIRTIO_NET_HDR_F_NEEDS_CSUM	1

#define VIRTIO_NET_S_LINK_UP		1

#define VIRTIO_NET_RX_QUEUE		0
#define VIRTIO_NET_TX_QUEUE		1
#define VIRTIO_NET_NQUEUES		2

#define VIRTIO_NET_DEV_ID		1

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct virtio_net_hdr_s {
	uint8_t flags;
	uint8_t gso_type;
	uint16_t hdr_len;
	uint16_t gso_size;
	uint16_t csum_start;
	uint16_t csum_offset;
} __attribute__((packed));

struct virtio_net_config_s {
	uint8_t mac[6];
	uint16_t status;
	uint16_t max_virtqueue_pairs;
	uint16_t mtu;
} __attribute__((packed));

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

int virtio_net_driver_initialize(uint32_t device_num);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_VIRTIO_VIRTIO_NET_H */
