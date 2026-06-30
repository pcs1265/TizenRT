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
#include <tinyara/irq.h>
#include <tinyara/kmalloc.h>

#include <debug.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "virtio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VIRTIO_MMIO_IRQ_BASE 48
#define VIRTIO_QUEUE_ALIGN   4096
#define VIRTIO_QUEUE_MAX     256

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint16_t virtio_queue_size(uint32_t qnum_max)
{
  uint16_t queue_num;

  if (qnum_max == 0)
    {
      return 0;
    }

  queue_num = qnum_max >= VIRTIO_QUEUE_MAX ?
              VIRTIO_QUEUE_MAX : (uint16_t)qnum_max;
  while (queue_num & (queue_num - 1))
    {
      queue_num &= queue_num - 1;
    }

  return queue_num;
}

static int virtio_mmio_interrupt(int irq, void *context, void *arg)
{
  struct virtio_device *vdev = arg;
  uint32_t int_status;
  unsigned int i;

  (void)irq;
  (void)context;

  int_status = virtio_mmio_get_interrupt_status(&vdev->mmio_dev);
  virtio_mmio_interrupt_ack(&vdev->mmio_dev, int_status);

  if ((int_status & VIRTIO_MMIO_INT_VRING) != 0 &&
      vdev->vrings_info != NULL)
    {
      for (i = 0; i < vdev->vrings_num; i++)
        {
          struct virtqueue *vq = vdev->vrings_info[i].vq;

          if (vq != NULL && vq->callback != NULL &&
              virtqueue_nused(vq) > 0)
            {
              vq->callback(vq);
            }
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int virtio_mmio_probe_driver(uint32_t device_num,
                             struct virtio_driver *driver)
{
  struct virtio_device *vdev;
  int ret;

  if (driver == NULL || driver->probe == NULL || driver->remove == NULL)
    {
      return -EINVAL;
    }

  vdev = kmm_zalloc(sizeof(*vdev));
  if (vdev == NULL)
    {
      return -ENOMEM;
    }

  ret = virtio_mmio_init(&vdev->mmio_dev, device_num);
  if (ret < 0)
    {
      kmm_free(vdev);
      return ret;
    }

  vdev->device_num = device_num;
  vdev->irq = VIRTIO_MMIO_IRQ_BASE + device_num;
  vdev->id.device = vdev->mmio_dev.device_id;
  vdev->id.vendor = vdev->mmio_dev.vendor_id;
  vdev->id.version = vdev->mmio_dev.version;

  if (driver->device != vdev->id.device)
    {
      virtio_mmio_deinit(&vdev->mmio_dev);
      kmm_free(vdev);
      return -ENODEV;
    }

  ret = irq_attach(vdev->irq, virtio_mmio_interrupt, vdev);
  if (ret < 0)
    {
      virtio_mmio_deinit(&vdev->mmio_dev);
      kmm_free(vdev);
      return ret;
    }

  virtio_set_status(vdev, VIRTIO_CONFIG_STATUS_ACK);

  ret = driver->probe(vdev);
  if (ret < 0)
    {
      irq_detach(vdev->irq);
      virtio_mmio_deinit(&vdev->mmio_dev);
      kmm_free(vdev);
      return ret;
    }

  up_enable_irq(vdev->irq);
  return OK;
}

int virtio_register_driver(struct virtio_driver *driver)
{
  (void)driver;
  return -ENOSYS;
}

void virtio_set_status(struct virtio_device *vdev, uint8_t status)
{
  uint8_t old;

  if (vdev == NULL)
    {
      return;
    }

  if (status == VIRTIO_CONFIG_STATUS_RESET)
    {
      virtio_mmio_set_status(&vdev->mmio_dev, status);
      return;
    }

  old = virtio_mmio_get_status(&vdev->mmio_dev);
  virtio_mmio_set_status(&vdev->mmio_dev, old | status);
}

uint8_t virtio_get_status(struct virtio_device *vdev)
{
  if (vdev == NULL)
    {
      return VIRTIO_CONFIG_STATUS_RESET;
    }

  return virtio_mmio_get_status(&vdev->mmio_dev);
}

void virtio_reset_device(struct virtio_device *vdev)
{
  if (vdev == NULL)
    {
      return;
    }

  virtio_mmio_set_status(&vdev->mmio_dev, VIRTIO_CONFIG_STATUS_RESET);
}

uint64_t virtio_get_features(struct virtio_device *vdev)
{
  uint64_t features;

  if (vdev == NULL)
    {
      return 0;
    }

  virtio_mmio_write32(&vdev->mmio_dev, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 0);
  features = virtio_mmio_read32(&vdev->mmio_dev,
                                VIRTIO_MMIO_DEVICE_FEATURES);

  virtio_mmio_write32(&vdev->mmio_dev, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 1);
  features |= (uint64_t)virtio_mmio_read32(&vdev->mmio_dev,
                                           VIRTIO_MMIO_DEVICE_FEATURES) << 32;

  return features;
}

void virtio_set_features(struct virtio_device *vdev, uint64_t features)
{
  if (vdev == NULL)
    {
      return;
    }

  virtio_mmio_write32(&vdev->mmio_dev, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
  virtio_mmio_write32(&vdev->mmio_dev, VIRTIO_MMIO_DRIVER_FEATURES,
                      (uint32_t)features);

  virtio_mmio_write32(&vdev->mmio_dev, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
  virtio_mmio_write32(&vdev->mmio_dev, VIRTIO_MMIO_DRIVER_FEATURES,
                      (uint32_t)(features >> 32));
}

void *virtio_zalloc_buf(struct virtio_device *vdev, size_t size,
                        size_t align)
{
  void *buf;

  (void)vdev;

  if (align > sizeof(void *))
    {
      buf = kmm_memalign(align, size);
    }
  else
    {
      buf = kmm_malloc(size);
    }

  if (buf != NULL)
    {
      memset(buf, 0, size);
    }

  return buf;
}

void virtio_free_buf(struct virtio_device *vdev, void *buf)
{
  (void)vdev;

  if (buf != NULL)
    {
      kmm_free(buf);
    }
}

int virtio_create_virtqueues(struct virtio_device *vdev, unsigned int flags,
                             unsigned int nvqs, const char *names[],
                             vq_callback callbacks[], void *callback_args[])
{
  unsigned int i;
  int ret;

  (void)flags;
  (void)callback_args;

  if (vdev == NULL || nvqs == 0)
    {
      return -EINVAL;
    }

  vdev->vrings_info = kmm_zalloc(sizeof(struct virtio_vring_info) * nvqs);
  if (vdev->vrings_info == NULL)
    {
      return -ENOMEM;
    }

  vdev->vrings_num = nvqs;

  for (i = 0; i < nvqs; i++)
    {
      struct virtio_vring_info *vrinfo = &vdev->vrings_info[i];
      struct virtqueue *vq;
      uint32_t qnum_max;
      uint16_t queue_num;

      virtio_mmio_select_queue(&vdev->mmio_dev, i);
      qnum_max = virtio_mmio_get_queue_num_max(&vdev->mmio_dev);
      queue_num = virtio_queue_size(qnum_max);
      if (queue_num == 0)
        {
          ret = -ENODEV;
          goto err_out;
        }

      vq = kmm_zalloc(sizeof(*vq));
      if (vq == NULL)
        {
          ret = -ENOMEM;
          goto err_out;
        }

      ret = virtq_init(vq, queue_num);
      if (ret < 0)
        {
          kmm_free(vq);
          goto err_out;
        }

      vq->vq_dev = vdev;
      vq->vq_queue_index = i;
      vq->vq_name = names != NULL ? names[i] : NULL;
      virtqueue_set_callback(vq, callbacks != NULL ? callbacks[i] : NULL);
      virtqueue_set_notify(vq, virtio_mmio_virtqueue_notify,
                           &vdev->mmio_dev);

      if (vdev->mmio_dev.version >= 2)
        {
          ret = virtio_mmio_setup_queue(&vdev->mmio_dev, i, queue_num,
                                        (uint64_t)(uintptr_t)vq->desc,
                                        (uint64_t)(uintptr_t)vq->avail,
                                        (uint64_t)(uintptr_t)vq->used);
        }
      else
        {
          ret = virtio_mmio_setup_queue_v1(&vdev->mmio_dev, i, queue_num,
                                           (uintptr_t)vq->desc);
        }

      if (ret < 0)
        {
          virtq_deinit(vq);
          kmm_free(vq);
          goto err_out;
        }

      vrinfo->vq = vq;
      vrinfo->notifyid = i;
      vrinfo->info.num_descs = queue_num;
      vrinfo->info.align = VIRTIO_QUEUE_ALIGN;
      vrinfo->info.vaddr = vq->desc;
    }

  return OK;

err_out:
  virtio_delete_virtqueues(vdev);
  return ret;
}

void virtio_delete_virtqueues(struct virtio_device *vdev)
{
  unsigned int i;

  if (vdev == NULL || vdev->vrings_info == NULL)
    {
      return;
    }

  for (i = 0; i < vdev->vrings_num; i++)
    {
      struct virtqueue *vq = vdev->vrings_info[i].vq;

      if (vq != NULL)
        {
          virtq_deinit(vq);
          kmm_free(vq);
        }
    }

  kmm_free(vdev->vrings_info);
  vdev->vrings_info = NULL;
  vdev->vrings_num = 0;
}
