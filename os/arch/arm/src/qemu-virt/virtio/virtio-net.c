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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#ifdef CONFIG_QEMU_VIRT_VIRTIO_NET

#include <tinyara/arch.h>
#include <tinyara/irq.h>
#include <tinyara/kmalloc.h>
#include <tinyara/net/if/ethernet.h>
#include <tinyara/netmgr/netdev_mgr.h>
#include <tinyara/wqueue.h>

#include <debug.h>
#include <errno.h>
#include <net/if.h>
#include <semaphore.h>
#include <stdint.h>
#include <string.h>

#include "virtio-net.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VIRTIO_NET_BUFSIZE	(CONFIG_NET_ETH_MTU + 64)
#define VIRTIO_NET_HDRSIZE	(sizeof(struct virtio_net_hdr_s))
#define VIRTIO_NET_TX_TIMEOUT	1000
#define VIRTIO_NET_QUEUE_MAX	256
#define VIRTIO_F_VERSION_1	0

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct virtio_net_rxbuf_s {
	struct virtio_net_hdr_s hdr;
	uint8_t data[VIRTIO_NET_BUFSIZE];
};

struct virtio_net_dev_s {
	virtio_mmio_dev_t mmio_dev;
	virtq_t rx_vq;
	virtq_t tx_vq;
	struct netdev *netdev;
	struct work_s irqwork;
	sem_t tx_lock;
	uint32_t features;
	uint32_t rx_count;
	int irq;
	bool ready;
	uint8_t mac[IFHWADDRLEN];
	struct virtio_net_rxbuf_s rxbuf[CONFIG_QEMU_VIRT_VIRTIO_NET_RX_BUFS];
	struct virtio_net_hdr_s txhdr;
	uint8_t txbuf[VIRTIO_NET_BUFSIZE];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct virtio_net_dev_s g_virtio_net;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int virtio_net_linkoutput(struct netdev *netdev, void *buf,
				 uint16_t len);

static struct nic_io_ops g_virtio_net_io_ops = {
	virtio_net_linkoutput,
	NULL
};

static int virtio_net_eth_init(struct netdev *dev)
{
	(void)dev;
	return OK;
}

static int virtio_net_eth_deinit(struct netdev *dev)
{
	(void)dev;
	return OK;
}

static int virtio_net_eth_enable(struct netdev *dev)
{
	(void)dev;
	return OK;
}

static int virtio_net_eth_disable(struct netdev *dev)
{
	(void)dev;
	return OK;
}

static struct ethernet_ops g_virtio_net_eth_ops = {
	virtio_net_eth_init,
	virtio_net_eth_deinit,
	virtio_net_eth_enable,
	virtio_net_eth_disable
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint16_t virtio_net_queue_size(uint32_t qnum_max)
{
	uint16_t queue_num;

	if (qnum_max == 0) {
		return 0;
	}

	queue_num = qnum_max >= VIRTIO_NET_QUEUE_MAX ?
		    VIRTIO_NET_QUEUE_MAX : (uint16_t)qnum_max;
	while (queue_num & (queue_num - 1)) {
		queue_num &= queue_num - 1;
	}

	return queue_num;
}

static int virtio_net_setup_queue(struct virtio_net_dev_s *dev,
				  uint32_t queue_sel, virtq_t *vq,
				  uint16_t *queue_num)
{
	uint32_t qnum_max;
	int ret;

	virtio_mmio_select_queue(&dev->mmio_dev, queue_sel);
	qnum_max = virtio_mmio_get_queue_num_max(&dev->mmio_dev);
	*queue_num = virtio_net_queue_size(qnum_max);
	if (*queue_num == 0) {
		return -ENODEV;
	}

	ret = virtq_init(vq, *queue_num);
	if (ret != OK) {
		return ret;
	}

	vq->vq_queue_index = queue_sel;
	virtqueue_set_notify(vq, virtio_mmio_virtqueue_notify,
			      &dev->mmio_dev);

	if (dev->mmio_dev.version >= 2) {
		ret = virtio_mmio_setup_queue(&dev->mmio_dev, queue_sel,
					      *queue_num,
					      (uint64_t)(uintptr_t)vq->desc,
					      (uint64_t)(uintptr_t)vq->avail,
					      (uint64_t)(uintptr_t)vq->used);
	} else {
		ret = virtio_mmio_setup_queue_v1(&dev->mmio_dev, queue_sel,
						 *queue_num,
						 (uintptr_t)vq->desc);
	}

	if (ret != OK) {
		virtq_deinit(vq);
		return ret;
	}

	return OK;
}

static int virtio_net_add_rxbuf(struct virtio_net_dev_s *dev, uint32_t index)
{
	struct virtqueue_buf vb[2];

	memset(&dev->rxbuf[index].hdr, 0, VIRTIO_NET_HDRSIZE);

	vb[0].buf = &dev->rxbuf[index].hdr;
	vb[0].len = VIRTIO_NET_HDRSIZE;
	vb[1].buf = dev->rxbuf[index].data;
	vb[1].len = VIRTIO_NET_BUFSIZE;

	if (virtqueue_add_buffer(&dev->rx_vq, vb, 0, 2,
				 &dev->rxbuf[index]) != OK) {
		return ERROR;
	}

	return OK;
}

static void virtio_net_rxfill(struct virtio_net_dev_s *dev)
{
	uint32_t i;

	for (i = 0; i < dev->rx_count; i++) {
		virtio_net_add_rxbuf(dev, i);
	}

	virtqueue_kick(&dev->rx_vq);
}

static bool virtio_net_vq_used_pending(virtq_t *vq)
{
	/* Memory barrier to ensure we see the latest used->idx written by
	 * the device (DMA). Without this, on ARM the CPU may read a stale
	 * cached value and miss completed buffers.
	 */

	__sync_synchronize();

	return vq->last_used_idx != vq->used->idx;
}

static void virtio_net_rx_worker(FAR void *arg)
{
	struct virtio_net_dev_s *dev = (struct virtio_net_dev_s *)arg;
	struct virtio_net_rxbuf_s *rxbuf;
	uint32_t pktlen;
	uint32_t used_len;
	uint16_t head;

	while (virtio_net_vq_used_pending(&dev->rx_vq)) {
		rxbuf = virtqueue_get_buffer(&dev->rx_vq, &used_len, &head);
		if (rxbuf == NULL || rxbuf < dev->rxbuf ||
		    rxbuf >= &dev->rxbuf[dev->rx_count] ||
		    used_len <= VIRTIO_NET_HDRSIZE) {
			continue;
		}

		pktlen = used_len - VIRTIO_NET_HDRSIZE;
		if (pktlen <= VIRTIO_NET_BUFSIZE && dev->netdev != NULL) {
			netdev_input(dev->netdev, rxbuf->data, pktlen);
		}

		/* NOTE: Do NOT set dev->rx_vq.free_head = head here!
		 * virtqueue_get_buffer() already reclaims the descriptor
		 * chain back to the free list. Overwriting free_head would
		 * corrupt the free list by pointing it to a now-in-use
		 * descriptor after virtio_net_add_rxbuf() allocates from it.
		 */

		virtio_net_add_rxbuf(dev, (uint32_t)(rxbuf - dev->rxbuf));
		virtqueue_kick(&dev->rx_vq);
	}
}

static int virtio_net_interrupt(int irq, FAR void *context, FAR void *arg)
{
	struct virtio_net_dev_s *dev = (struct virtio_net_dev_s *)arg;
	uint32_t int_status;

	(void)irq;
	(void)context;

	int_status = virtio_mmio_get_interrupt_status(&dev->mmio_dev);
	virtio_mmio_interrupt_ack(&dev->mmio_dev, int_status);

	if (int_status & VIRTIO_MMIO_INT_VRING) {
		work_queue(HPWORK, &dev->irqwork, virtio_net_rx_worker, dev, 0);
	}

	return OK;
}

static int virtio_net_linkoutput(struct netdev *netdev, void *buf,
				 uint16_t len)
{
	struct virtio_net_dev_s *dev = (struct virtio_net_dev_s *)netdev->priv;
	struct virtqueue_buf vb[2];
	int timeout;
	int ret;

	if (dev == NULL || !dev->ready || buf == NULL ||
	    len == 0 || len > VIRTIO_NET_BUFSIZE) {
		return -EINVAL;
	}

	sem_wait(&dev->tx_lock);

	/* Reclaim any previously completed TX descriptors.
	 * This is essential to prevent TX queue exhaustion — without
	 * reclamation, descriptors are consumed but never returned to the
	 * free list, eventually causing virtqueue_add_buffer() to fail
	 * with -ENOSPC.
	 */

	while (virtio_net_vq_used_pending(&dev->tx_vq)) {
		virtqueue_get_buffer(&dev->tx_vq, NULL, NULL);
	}

	memset(&dev->txhdr, 0, VIRTIO_NET_HDRSIZE);
	memcpy(dev->txbuf, buf, len);

	vb[0].buf = &dev->txhdr;
	vb[0].len = VIRTIO_NET_HDRSIZE;
	vb[1].buf = dev->txbuf;
	vb[1].len = len;

	ret = virtqueue_add_buffer(&dev->tx_vq, vb, 2, 0, NULL);
	if (ret != OK) {
		sem_post(&dev->tx_lock);
		return -EIO;
	}

	virtqueue_kick(&dev->tx_vq);

	for (timeout = 0; timeout < VIRTIO_NET_TX_TIMEOUT; timeout++) {
		if (virtio_net_vq_used_pending(&dev->tx_vq)) {
			/* Reclaim the completed TX descriptor to prevent
			 * queue exhaustion on subsequent sends.
			 */

			virtqueue_get_buffer(&dev->tx_vq, NULL, NULL);
			sem_post(&dev->tx_lock);
			return OK;
		}

		up_mdelay(1);
	}

	sem_post(&dev->tx_lock);
	return -ETIMEDOUT;
}

static void virtio_net_read_mac(struct virtio_net_dev_s *dev)
{
	uint32_t i;

	if (dev->features & VIRTIO_NET_F_MAC) {
		for (i = 0; i < IFHWADDRLEN; i++) {
			dev->mac[i] = virtio_mmio_read8(&dev->mmio_dev,
							VIRTIO_MMIO_CONFIG + i);
		}
	} else {
		dev->mac[0] = 0x42;
		dev->mac[1] = 0x54;
		dev->mac[2] = 0x00;
		dev->mac[3] = 0x12;
		dev->mac[4] = 0x34;
		dev->mac[5] = 0x56;
	}
}

static int virtio_net_register_netdev(struct virtio_net_dev_s *dev)
{
	struct netdev_config config;

	memset(&config, 0, sizeof(config));
	config.ops = &g_virtio_net_io_ops;
	config.flag = NM_FLAG_ETHARP | NM_FLAG_ETHERNET | NM_FLAG_BROADCAST |
		      NM_FLAG_IGMP;
	config.mtu = CONFIG_NET_ETH_MTU;
	config.hwaddr_len = IFHWADDRLEN;
	memcpy(config.hwaddr, dev->mac, IFHWADDRLEN);
	config.is_default = 1;
	config.type = NM_ETHERNET;
	config.t_ops.eth = &g_virtio_net_eth_ops;
	config.priv = dev;

	dev->netdev = netdev_register(&config);
	if (dev->netdev == NULL) {
		return -ENODEV;
	}

	netdev_set_hwaddr(dev->netdev, dev->mac, IFHWADDRLEN);
	return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int virtio_net_driver_initialize(uint32_t device_num)
{
	struct virtio_net_dev_s *dev = &g_virtio_net;
	uint32_t device_features;
	uint32_t driver_features = 0;
	uint16_t rx_queue_num;
	uint16_t tx_queue_num;
	uint8_t status;
	int ret;

	memset(dev, 0, sizeof(*dev));

	sem_init(&dev->tx_lock, 0, 1);
	dev->irq = 48 + device_num;

	ret = virtio_mmio_init(&dev->mmio_dev, device_num);
	if (ret != OK) {
		return ret;
	}

	if (dev->mmio_dev.device_id != VIRTIO_NET_DEV_ID) {
		virtio_mmio_deinit(&dev->mmio_dev);
		return -ENODEV;
	}

	virtio_mmio_set_status(&dev->mmio_dev, VIRTIO_CONFIG_STATUS_ACK);
	virtio_mmio_set_status(&dev->mmio_dev,
			       VIRTIO_CONFIG_STATUS_ACK |
			       VIRTIO_CONFIG_STATUS_DRIVER);

	device_features = virtio_mmio_get_device_features(&dev->mmio_dev);
	if (device_features & VIRTIO_NET_F_MAC) {
		driver_features |= VIRTIO_NET_F_MAC;
	}
	if (device_features & VIRTIO_NET_F_STATUS) {
		driver_features |= VIRTIO_NET_F_STATUS;
	}

	dev->features = driver_features;
	virtio_mmio_set_driver_features(&dev->mmio_dev, driver_features);

	if (dev->mmio_dev.version >= 2) {
		virtio_mmio_write32(&dev->mmio_dev,
				    VIRTIO_MMIO_DEVICE_FEATURES_SEL, 1);
		virtio_mmio_read32(&dev->mmio_dev, VIRTIO_MMIO_DEVICE_FEATURES);
		virtio_mmio_write32(&dev->mmio_dev,
				    VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
		virtio_mmio_write32(&dev->mmio_dev, VIRTIO_MMIO_DRIVER_FEATURES,
				    (1 << VIRTIO_F_VERSION_1));
	}

	virtio_mmio_set_status(&dev->mmio_dev,
			       VIRTIO_CONFIG_STATUS_ACK |
			       VIRTIO_CONFIG_STATUS_DRIVER |
			       VIRTIO_CONFIG_STATUS_FEATURES_OK);
	status = virtio_mmio_get_status(&dev->mmio_dev);
	if (!(status & VIRTIO_CONFIG_STATUS_FEATURES_OK)) {
		virtio_mmio_set_status(&dev->mmio_dev, VIRTIO_CONFIG_STATUS_FAILED);
		virtio_mmio_deinit(&dev->mmio_dev);
		return -ENOTSUP;
	}

	virtio_net_read_mac(dev);

	ret = virtio_net_setup_queue(dev, VIRTIO_NET_RX_QUEUE, &dev->rx_vq,
				     &rx_queue_num);
	if (ret != OK) {
		virtio_mmio_deinit(&dev->mmio_dev);
		return ret;
	}

	ret = virtio_net_setup_queue(dev, VIRTIO_NET_TX_QUEUE, &dev->tx_vq,
				     &tx_queue_num);
	if (ret != OK) {
		virtq_deinit(&dev->rx_vq);
		virtio_mmio_deinit(&dev->mmio_dev);
		return ret;
	}

	dev->rx_count = CONFIG_QEMU_VIRT_VIRTIO_NET_RX_BUFS;
	if (dev->rx_count > (rx_queue_num / 2)) {
		dev->rx_count = rx_queue_num / 2;
	}

	ret = irq_attach(dev->irq, virtio_net_interrupt, dev);
	if (ret != OK) {
		virtq_deinit(&dev->tx_vq);
		virtq_deinit(&dev->rx_vq);
		virtio_mmio_deinit(&dev->mmio_dev);
		return ret;
	}

	ret = virtio_net_register_netdev(dev);
	if (ret != OK) {
		virtq_deinit(&dev->tx_vq);
		virtq_deinit(&dev->rx_vq);
		virtio_mmio_deinit(&dev->mmio_dev);
		return ret;
	}

	virtio_mmio_set_status(&dev->mmio_dev,
			       VIRTIO_CONFIG_STATUS_ACK |
			       VIRTIO_CONFIG_STATUS_DRIVER |
			       VIRTIO_CONFIG_STATUS_FEATURES_OK |
			       VIRTIO_CONFIG_STATUS_DRIVER_OK);

	virtio_net_rxfill(dev);

	up_enable_irq(dev->irq);
	dev->ready = true;

	lldbg("virtio-net: registered %s mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
	      dev->netdev->ifname, dev->mac[0], dev->mac[1], dev->mac[2],
	      dev->mac[3], dev->mac[4], dev->mac[5]);

	return OK;
}

#endif /* CONFIG_QEMU_VIRT_VIRTIO_NET */
