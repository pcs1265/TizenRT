/****************************************************************************
 * drivers/virtio/virtio-serial.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
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

#include <debug.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <tinyara/fs/fs.h>
#include <tinyara/kmalloc.h>
#include <tinyara/serial/serial.h>
#include <tinyara/spinlock.h>

#include "virtio.h"

#include "virtio-serial.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VIRTIO_SERIAL_RX           0
#define VIRTIO_SERIAL_TX           1
#define VIRTIO_SERIAL_NUM          2

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct virtio_serial_priv_s
{
  /* Virtio device information */

  FAR struct virtio_device *vdev;

  /* TinyAra uart device information */

  FAR struct uart_dev_s     udev;
  char                      name[NAME_MAX];
  spinlock_t                lock;
  FAR char                 *rxbuf;
  uint16_t                  rxlen;
  uint16_t                  rxpos;
  uint16_t                  txqueued;
  bool                      rxposted;
  bool                      rxenabled;
  bool                      txbusy;
  bool                      txenabled;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Uart operation functions */

static int  virtio_serial_setup(FAR struct uart_dev_s *dev);
static void virtio_serial_shutdown(FAR struct uart_dev_s *dev);
static int  virtio_serial_attach(FAR struct uart_dev_s *dev);
static void virtio_serial_detach(FAR struct uart_dev_s *dev);
static int  virtio_serial_ioctl(FAR struct uart_dev_s *dev, int cmd,
                                unsigned long arg);
static int  virtio_serial_receive(FAR struct uart_dev_s *dev,
                                  FAR unsigned int *status);
static void virtio_serial_rxint(FAR struct uart_dev_s *dev, bool enable);
static bool virtio_serial_rxavailable(FAR struct uart_dev_s *dev);
static void virtio_serial_send(FAR struct uart_dev_s *dev, int ch);
static void virtio_serial_txint(FAR struct uart_dev_s *dev, bool enable);
static bool virtio_serial_txready(FAR struct uart_dev_s *dev);
static bool virtio_serial_txempty(FAR struct uart_dev_s *dev);

/* Other functions */

static uint16_t virtio_serial_advance(uint16_t pos, uint16_t len,
                                      uint16_t size);
static uint16_t virtio_serial_txbufs(FAR struct uart_dev_s *dev,
                                     FAR struct virtqueue_buf *vb,
                                     FAR int *num);
static void virtio_serial_start_tx(FAR struct virtio_serial_priv_s *priv);
static void virtio_serial_post_rxbuf(FAR struct virtio_serial_priv_s *priv);
static void virtio_serial_rxready(FAR struct virtqueue *vq);
static void virtio_serial_txdone(FAR struct virtqueue *vq);

static int  virtio_serial_probe(FAR struct virtio_device *vdev);
static void virtio_serial_remove(FAR struct virtio_device *vdev);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct virtio_driver g_virtio_serial_driver =
{
  LIST_INITIAL_VALUE(g_virtio_serial_driver.node), /* node */
  VIRTIO_ID_CONSOLE,                               /* device id */
  virtio_serial_probe,                             /* probe */
  virtio_serial_remove,                            /* remove */
};

static struct virtio_driver g_virtio_rprocserial_driver =
{
  LIST_INITIAL_VALUE(g_virtio_rprocserial_driver.node), /* node */
  VIRTIO_ID_RPROC_SERIAL,                               /* device id */
  virtio_serial_probe,                                  /* probe */
  virtio_serial_remove,                                 /* remove */
};

static const struct uart_ops_s g_virtio_serial_ops =
{
  .setup       = virtio_serial_setup,
  .shutdown    = virtio_serial_shutdown,
  .attach      = virtio_serial_attach,
  .detach      = virtio_serial_detach,
  .ioctl       = virtio_serial_ioctl,
  .receive     = virtio_serial_receive,
  .rxint       = virtio_serial_rxint,
  .rxavailable = virtio_serial_rxavailable,
#ifdef CONFIG_SERIAL_IFLOWCONTROL
  .rxflowcontrol = NULL,
#endif
  .send        = virtio_serial_send,
  .txint       = virtio_serial_txint,
  .txready     = virtio_serial_txready,
  .txempty     = virtio_serial_txempty,
};

static int g_virtio_serial_idx = 0;

#ifdef CONFIG_QEMU_VIRT_VIRTIO_SERIAL_CONSOLE
static struct uart_dev_s *g_virtio_console;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: virtio_serial_setup
 *
 * Description:
 *   Configure the UART baud, bits, parity, fifos, etc. This
 *   method is called the first time that the serial port is
 *   opened.
 *
 ****************************************************************************/

static int virtio_serial_setup(FAR struct uart_dev_s *dev)
{
  return OK;
}

/****************************************************************************
 * Name: virtio_serial_shutdown
 *
 * Description:
 *   Disable the UART.  This method is called when the serial
 *   port is closed
 *
 ****************************************************************************/

static void virtio_serial_shutdown(FAR struct uart_dev_s *dev)
{
  /* Nothing */
}

/****************************************************************************
 * Name: virtio_serial_attach
 *
 * Description:
 *   Configure the UART to operation in interrupt driven mode.  This method
 *   is called when the serial port is opened.  Normally, this is just after
 *   the setup() method is called, however, the serial console may operate in
 *   a non-interrupt driven mode during the boot phase.
 *
 *   RX and TX interrupts are not enabled when by the attach method (unless
 *   the hardware supports multiple levels of interrupt enabling).  The RX
 *   and TX interrupts are not enabled until the txint() and rxint() methods
 *   are called.
 *
 ****************************************************************************/

static int virtio_serial_attach(FAR struct uart_dev_s *dev)
{
  FAR struct virtio_serial_priv_s *priv = dev->priv;
  FAR struct virtqueue *rxvq = priv->vdev->vrings_info[VIRTIO_SERIAL_RX].vq;
  FAR struct virtqueue *txvq = priv->vdev->vrings_info[VIRTIO_SERIAL_TX].vq;

  virtqueue_enable_cb(rxvq);
  virtqueue_enable_cb(txvq);
  return 0;
}

/****************************************************************************
 * Name: virtio_serial_detach
 *
 * Description:
 *   Detach UART interrupts.  This method is called when the serial port is
 *   closed normally just before the shutdown method is called.  The
 *   exception is the serial console which is never shutdown.
 *
 ****************************************************************************/

static void virtio_serial_detach(FAR struct uart_dev_s *dev)
{
  FAR struct virtio_serial_priv_s *priv = dev->priv;
  FAR struct virtqueue *rxvq = priv->vdev->vrings_info[VIRTIO_SERIAL_RX].vq;
  FAR struct virtqueue *txvq = priv->vdev->vrings_info[VIRTIO_SERIAL_TX].vq;

  virtqueue_disable_cb(rxvq);
  virtqueue_disable_cb(txvq);
}

/****************************************************************************
 * Name: virtio_serial_ioctl
 *
 * Description:
 *   All ioctl calls will be routed through this method
 *
 ****************************************************************************/

static int virtio_serial_ioctl(FAR struct uart_dev_s *dev, int cmd,
                               unsigned long arg)
{
  return -ENOTTY;
}

/****************************************************************************
 * Name: virtio_serial_receive
 ****************************************************************************/

static int virtio_serial_receive(FAR struct uart_dev_s *dev,
                                 FAR unsigned int *status)
{
  FAR struct virtio_serial_priv_s *priv = dev->priv;
  irqstate_t flags;
  int ch = 0;

  if (status != NULL)
    {
      *status = 0;
    }

  flags = spin_lock_irqsave(&priv->lock);
  if (priv->rxpos < priv->rxlen)
    {
      ch = priv->rxbuf[priv->rxpos++];
      if (priv->rxpos >= priv->rxlen)
        {
          priv->rxpos = 0;
          priv->rxlen = 0;
        }
    }

  spin_unlock_irqrestore(&priv->lock, flags);
  return ch & 0xff;
}

/****************************************************************************
 * Name: virtio_serial_rxint
 *
 * Description:
 *   Call to enable or disable RX interrupts
 *
 ****************************************************************************/

static void virtio_serial_rxint(FAR struct uart_dev_s *dev, bool enable)
{
  FAR struct virtio_serial_priv_s *priv = dev->priv;
  irqstate_t flags;

  flags = spin_lock_irqsave(&priv->lock);
  priv->rxenabled = enable;
  spin_unlock_irqrestore(&priv->lock, flags);

  if (enable)
    {
      virtio_serial_post_rxbuf(priv);
    }
}

/****************************************************************************
 * Name: virtio_serial_rxavailable
 ****************************************************************************/

static bool virtio_serial_rxavailable(FAR struct uart_dev_s *dev)
{
  FAR struct virtio_serial_priv_s *priv = dev->priv;
  irqstate_t flags;
  bool available;

  flags = spin_lock_irqsave(&priv->lock);
  available = priv->rxpos < priv->rxlen;
  spin_unlock_irqrestore(&priv->lock, flags);

  return available;
}

/****************************************************************************
 * Name: virtio_serial_send
 *
 * Description:
 *   Queue one byte from direct console output. TinyAra's normal write()
 *   path fills xmit.buffer before txint(), so this driver does not use
 *   uart_xmitchars() for regular TX.
 *
 ****************************************************************************/

static void virtio_serial_send(FAR struct uart_dev_s *dev, int ch)
{
  FAR struct virtio_serial_priv_s *priv = dev->priv;
  irqstate_t flags;
  int nexthead;
  bool queued = false;

  flags = spin_lock_irqsave(&priv->lock);

  nexthead = dev->xmit.head + 1;
  if (nexthead >= dev->xmit.size)
    {
      nexthead = 0;
    }

  if (nexthead != dev->xmit.tail)
    {
      /* No.. not full.  Add the character to the TX buffer and return. */

      dev->xmit.buffer[dev->xmit.head] = ch;
      dev->xmit.head = nexthead;
      priv->txenabled = true;
      queued = true;
    }

  spin_unlock_irqrestore(&priv->lock, flags);

  if (queued)
    {
      virtio_serial_start_tx(priv);
    }
}

/****************************************************************************
 * Name: virtio_serial_txint
 *
 * Description:
 *   Call to enable or disable TX interrupts
 *
 ****************************************************************************/

static void virtio_serial_txint(FAR struct uart_dev_s *dev, bool enable)
{
  FAR struct virtio_serial_priv_s *priv = dev->priv;
  irqstate_t flags;

  flags = spin_lock_irqsave(&priv->lock);
  priv->txenabled = enable;
  spin_unlock_irqrestore(&priv->lock, flags);

  if (enable)
    {
      /* Drain the upper-half xmit ring as bulk virtqueue descriptors. */

      virtio_serial_start_tx(priv);
    }
}

/****************************************************************************
 * Name: uart_txready
 *
 * Description:
 *   Return true if the tranmsit fifo is not full
 *
 ****************************************************************************/

static bool virtio_serial_txready(FAR struct uart_dev_s *dev)
{
  FAR struct virtio_serial_priv_s *priv = dev->priv;

  return !priv->txbusy;
}

/****************************************************************************
 * Name: virtio_serial_txempty
 *
 * Description:
 *   Return true if the transmit fifo is empty
 *
 ****************************************************************************/

static bool virtio_serial_txempty(FAR struct uart_dev_s *dev)
{
  FAR struct virtio_serial_priv_s *priv = dev->priv;

  return !priv->txbusy && dev->xmit.head == dev->xmit.tail;
}

/****************************************************************************
 * Name: virtio_serial_advance
 ****************************************************************************/

static uint16_t virtio_serial_advance(uint16_t pos, uint16_t len,
                                      uint16_t size)
{
  pos += len;
  while (pos >= size)
    {
      pos -= size;
    }

  return pos;
}

/****************************************************************************
 * Name: virtio_serial_txbufs
 ****************************************************************************/

static uint16_t virtio_serial_txbufs(FAR struct uart_dev_s *dev,
                                     FAR struct virtqueue_buf *vb,
                                     FAR int *num)
{
  uint32_t head;
  uint32_t tail;
  uint32_t size;
  uint32_t first;
  uint32_t second;

  head = dev->xmit.head;
  tail = dev->xmit.tail;
  size = dev->xmit.size;

  if (size == 0 || head == tail)
    {
      return 0;
    }

  if (tail < head)
    {
      first = head - tail;
      vb[0].buf = &dev->xmit.buffer[tail];
      vb[0].len = first;
      *num = 1;
      return first;
    }

  first = size - tail;
  second = head;

  vb[0].buf = &dev->xmit.buffer[tail];
  vb[0].len = first;
  if (second != 0)
    {
      vb[1].buf = dev->xmit.buffer;
      vb[1].len = second;
      *num = 2;
      return first + second;
    }

  *num = 1;
  return first;
}

/****************************************************************************
 * Name: virtio_serial_start_tx
 ****************************************************************************/

static void virtio_serial_start_tx(FAR struct virtio_serial_priv_s *priv)
{
  FAR struct virtqueue *vq = priv->vdev->vrings_info[VIRTIO_SERIAL_TX].vq;
  struct virtqueue_buf vb[2];
  irqstate_t flags;
  uint16_t len;
  int num;
  int ret;

  flags = spin_lock_irqsave(&priv->lock);
  if (priv->txbusy)
    {
      spin_unlock_irqrestore(&priv->lock, flags);
      return;
    }

  len = virtio_serial_txbufs(&priv->udev, vb, &num);
  if (len == 0)
    {
      spin_unlock_irqrestore(&priv->lock, flags);
      return;
    }

  ret = virtqueue_add_buffer(vq, vb, num, 0, (FAR void *)(uintptr_t)len);
  if (ret == OK)
    {
      priv->txbusy = true;
      priv->txqueued = len;
      virtqueue_kick(vq);
    }

  spin_unlock_irqrestore(&priv->lock, flags);
}

/****************************************************************************
 * Name: virtio_serial_post_rxbuf
 ****************************************************************************/

static void virtio_serial_post_rxbuf(FAR struct virtio_serial_priv_s *priv)
{
  FAR struct virtqueue *vq = priv->vdev->vrings_info[VIRTIO_SERIAL_RX].vq;
  struct virtqueue_buf vb;
  irqstate_t flags;
  int ret;

  flags = spin_lock_irqsave(&priv->lock);
  if (priv->rxposted || priv->rxlen != 0)
    {
      spin_unlock_irqrestore(&priv->lock, flags);
      return;
    }

  vb.buf = priv->rxbuf;
  vb.len = CONFIG_QEMU_VIRT_VIRTIO_SERIAL_BUFSIZE;

  ret = virtqueue_add_buffer(vq, &vb, 0, 1, priv->rxbuf);
  if (ret == OK)
    {
      priv->rxposted = true;
      virtqueue_kick(vq);
    }

  spin_unlock_irqrestore(&priv->lock, flags);
}

static void virtio_serial_rxready(FAR struct virtqueue *vq)
{
  FAR struct virtio_serial_priv_s *priv = vq->vq_dev->priv;
  FAR void *cookie;
  bool restart;
  irqstate_t flags;
  uint32_t len;

  /* Received data has already been written into the staging RX buffer. */

  flags = spin_lock_irqsave(&priv->lock);
  cookie = virtqueue_get_buffer(vq, &len, NULL);
  if (cookie == NULL)
    {
      spin_unlock_irqrestore(&priv->lock, flags);
      return;
    }

  priv->rxposted = false;
  if (len > CONFIG_QEMU_VIRT_VIRTIO_SERIAL_BUFSIZE)
    {
      len = CONFIG_QEMU_VIRT_VIRTIO_SERIAL_BUFSIZE;
    }

  priv->rxpos = 0;
  priv->rxlen = len;
  restart = priv->rxenabled;
  spin_unlock_irqrestore(&priv->lock, flags);

  if (len != 0)
    {
      uart_recvchars(&priv->udev);
    }

  if (restart)
    {
      virtio_serial_post_rxbuf(priv);
    }
}

/****************************************************************************
 * Name: virtio_serial_txdone
 *
 * Description:
 *   The virt serial transmit virtqueue callback function
 *
 ****************************************************************************/

static void virtio_serial_txdone(FAR struct virtqueue *vq)
{
  FAR struct virtio_serial_priv_s *priv = vq->vq_dev->priv;
  FAR void *cookie;
  bool restart;
  bool notify;
  irqstate_t flags;
  uint16_t len;

  /* Reclaim the completed transmit buffer and resume upper-half TX. */

  flags = spin_lock_irqsave(&priv->lock);
  cookie = virtqueue_get_buffer(vq, NULL, NULL);
  if (cookie == NULL)
    {
      spin_unlock_irqrestore(&priv->lock, flags);
      return;
    }

  len = (uint16_t)(uintptr_t)cookie;
  if (len > priv->txqueued)
    {
      len = priv->txqueued;
    }

  if (len != 0)
    {
      priv->udev.xmit.tail =
        virtio_serial_advance(priv->udev.xmit.tail, len,
                              priv->udev.xmit.size);
    }

  priv->txbusy = false;
  priv->txqueued = 0;
  notify = len != 0 && priv->udev.sent != NULL;
  restart = priv->txenabled &&
            priv->udev.xmit.head != priv->udev.xmit.tail;
  spin_unlock_irqrestore(&priv->lock, flags);

  if (notify)
    {
      priv->udev.sent(&priv->udev);
    }

  if (restart)
    {
      virtio_serial_start_tx(priv);
    }
}

/****************************************************************************
 * Name: virtio_serial_init
 ****************************************************************************/

static int virtio_serial_init(FAR struct virtio_serial_priv_s *priv,
                              FAR struct virtio_device *vdev)
{
  FAR const char *vqnames[VIRTIO_SERIAL_NUM];
  vq_callback callbacks[VIRTIO_SERIAL_NUM];
  FAR struct uart_dev_s *udev;
  int ret;

  priv->vdev = vdev;
  vdev->priv = priv;
  spin_initialize(&priv->lock, SP_UNLOCKED);

  /* Uart device buffer and ops init */

  udev              = &priv->udev;
  udev->priv        = priv;
  udev->ops         = &g_virtio_serial_ops;
  udev->recv.size   = CONFIG_QEMU_VIRT_VIRTIO_SERIAL_BUFSIZE;
  udev->recv.buffer = virtio_zalloc_buf(vdev, udev->recv.size, 16);
  if (udev->recv.buffer == NULL)
    {
      vrterr("No enough memory\n");
      return -ENOMEM;
    }

  udev->xmit.size   = CONFIG_QEMU_VIRT_VIRTIO_SERIAL_BUFSIZE;
  udev->xmit.buffer = virtio_zalloc_buf(vdev, udev->xmit.size, 16);
  if (udev->xmit.buffer == NULL)
    {
      vrterr("No enough memory\n");
      ret = -ENOMEM;
      goto err_with_recv;
    }

  priv->rxbuf = virtio_zalloc_buf(vdev,
                                  CONFIG_QEMU_VIRT_VIRTIO_SERIAL_BUFSIZE,
                                  16);
  if (priv->rxbuf == NULL)
    {
      vrterr("No enough memory\n");
      ret = -ENOMEM;
      goto err_with_xmit;
    }

  /* Initialize the virtio device */

  virtio_set_status(vdev, VIRTIO_CONFIG_STATUS_DRIVER);
  virtio_set_features(vdev, 0);
  virtio_set_status(vdev, VIRTIO_CONFIG_FEATURES_OK);

  vqnames[VIRTIO_SERIAL_RX]   = "virtio_serial_rx";
  vqnames[VIRTIO_SERIAL_TX]   = "virtio_serial_tx";
  callbacks[VIRTIO_SERIAL_RX] = virtio_serial_rxready;
  callbacks[VIRTIO_SERIAL_TX] = virtio_serial_txdone;
  ret = virtio_create_virtqueues(vdev, 0, VIRTIO_SERIAL_NUM, vqnames,
                                 callbacks, NULL);
  if (ret < 0)
    {
      vrterr("virtio_device_create_virtqueue failed, ret=%d\n", ret);
      goto err_with_rxbuf;
    }

  virtio_set_status(vdev, VIRTIO_CONFIG_STATUS_DRIVER_OK);
  return OK;

err_with_rxbuf:
  virtio_free_buf(vdev, priv->rxbuf);
err_with_xmit:
  virtio_free_buf(vdev, udev->xmit.buffer);
err_with_recv:
  virtio_free_buf(vdev, udev->recv.buffer);
  virtio_reset_device(vdev);
  return ret;
}

/****************************************************************************
 * Name: virtio_serial_uninit
 ****************************************************************************/

static void virtio_serial_uninit(FAR struct virtio_serial_priv_s *priv)
{
  FAR struct virtio_device *vdev = priv->vdev;

  virtio_reset_device(vdev);
  virtio_delete_virtqueues(vdev);
  virtio_free_buf(vdev, priv->rxbuf);
  virtio_free_buf(vdev, priv->udev.xmit.buffer);
  virtio_free_buf(vdev, priv->udev.recv.buffer);
}

/****************************************************************************
 * Name: virtio_serial_uart_register
 ****************************************************************************/

static int virtio_serial_uart_register(FAR struct virtio_serial_priv_s *priv)
{
  FAR const char *name = CONFIG_QEMU_VIRT_VIRTIO_SERIAL_NAME;
  bool found = false;
  int start = 0;
  int ret;
  int i;
  int j;

  for (i = 0, j = 0; name[start] != '\0'; i++)
    {
      if (name[i] == ';' || name[i] == '\0')
        {
          if (j++ == g_virtio_serial_idx)
            {
              found = true;
              break;
            }

          start = i + 1;
        }
    }

  if (found)
    {
      snprintf(priv->name, NAME_MAX, "/dev/%.*s", i - start, &name[start]);
    }
  else
    {
      snprintf(priv->name, NAME_MAX, "/dev/ttyV%d", g_virtio_serial_idx);
    }

  ret = uart_register(priv->name, &priv->udev);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_QEMU_VIRT_VIRTIO_SERIAL_CONSOLE
  if (g_virtio_console == NULL)
    {
      DEBUGVERIFY(uart_register("/dev/console", &priv->udev));
      g_virtio_console = &priv->udev;
      g_virtio_console->isconsole = true;
    }
#endif

  g_virtio_serial_idx++;
  return ret;
}

/****************************************************************************
 * Name: virtio_serial_probe
 ****************************************************************************/

static int virtio_serial_probe(FAR struct virtio_device *vdev)
{
  FAR struct virtio_serial_priv_s *priv;
  int ret;

  /* Alloc the virtio serial driver and uart buffer */

  priv = kmm_zalloc(sizeof(*priv));
  if (priv == NULL)
    {
      vrterr("No enough memory\n");
      return -ENOMEM;
    }

  ret = virtio_serial_init(priv, vdev);
  if (ret < 0)
    {
      vrterr("virtio_serial_init failed, ret=%d\n", ret);
      goto err_with_priv;
    }

  /* Uart driver register */

  ret = virtio_serial_uart_register(priv);
  if (ret < 0)
    {
      vrterr("uart_register failed, ret=%d\n", ret);
      goto err_with_init;
    }

  return ret;

err_with_init:
  virtio_serial_uninit(priv);
err_with_priv:
  kmm_free(priv);
  return ret;
}

/****************************************************************************
 * Name: virtio_serial_remove
 ****************************************************************************/

static void virtio_serial_remove(FAR struct virtio_device *vdev)
{
  FAR struct virtio_serial_priv_s *priv = vdev->priv;

  unregister_driver(priv->name);
  virtio_serial_uninit(priv);
  kmm_free(priv);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: virtio_register_serial_driver
 ****************************************************************************/

int virtio_register_serial_driver(void)
{
  return virtio_serial_driver_initialize(
           CONFIG_QEMU_VIRT_VIRTIO_SERIAL_DEVICE_NUM);
}

/****************************************************************************
 * Name: virtio_serial_driver_initialize
 ****************************************************************************/

int virtio_serial_driver_initialize(uint32_t device_num)
{
  int ret;

  ret = virtio_mmio_probe_driver(device_num, &g_virtio_serial_driver);
  if (ret == -ENODEV)
    {
      ret = virtio_mmio_probe_driver(device_num, &g_virtio_rprocserial_driver);
    }

  return ret;
}

#ifdef CONFIG_QEMU_VIRT_VIRTIO_SERIAL_CONSOLE
/****************************************************************************
 * Name: up_putc
 ****************************************************************************/

int up_putc(int ch)
{
  if (g_virtio_console != NULL)
    {
      virtio_serial_send(g_virtio_console, ch);
    }

  return ch;
}
#endif
