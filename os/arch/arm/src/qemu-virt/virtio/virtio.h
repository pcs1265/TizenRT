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

#ifndef __ARCH_ARM_SRC_QEMU_VIRT_VIRTIO_VIRTIO_H
#define __ARCH_ARM_SRC_QEMU_VIRT_VIRTIO_VIRTIO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <debug.h>
#include <stdint.h>
#include <stddef.h>

#include "virtio-mmio.h"
#include "virtio-queue.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VIRTIO_ID_NET              1
#define VIRTIO_ID_BLOCK            2
#define VIRTIO_ID_CONSOLE          3
#define VIRTIO_ID_RNG              4
#define VIRTIO_ID_RPROC_SERIAL     11

#define VIRTIO_CONFIG_FEATURES_OK      VIRTIO_CONFIG_STATUS_FEATURES_OK

#ifdef CONFIG_QEMU_VIRT_VIRTIO_SERIAL
#  ifndef CONFIG_DRIVERS_VIRTIO_SERIAL
#    define CONFIG_DRIVERS_VIRTIO_SERIAL 1
#  endif

#  ifndef CONFIG_DRIVERS_VIRTIO_SERIAL_NAME
#    define CONFIG_DRIVERS_VIRTIO_SERIAL_NAME \
            CONFIG_QEMU_VIRT_VIRTIO_SERIAL_NAME
#  endif

#  ifndef CONFIG_DRIVERS_VIRTIO_SERIAL_BUFSIZE
#    define CONFIG_DRIVERS_VIRTIO_SERIAL_BUFSIZE \
            CONFIG_QEMU_VIRT_VIRTIO_SERIAL_BUFSIZE
#  endif
#endif

#ifndef LIST_INITIAL_VALUE
#  define LIST_INITIAL_VALUE(name) { &(name), &(name) }
#endif

#ifndef vrterr
#  define vrterr(format, ...) lldbg("virtio: " format, ##__VA_ARGS__)
#endif

#ifndef vrtinfo
#  define vrtinfo(format, ...) lldbg("virtio: " format, ##__VA_ARGS__)
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct list_node
{
  struct list_node *flink;
  struct list_node *blink;
};

struct virtio_device_id
{
  uint32_t device;
  uint32_t vendor;
  uint32_t version;
};

struct vring_alloc_info
{
  uint16_t num_descs;
  uint16_t align;
  void    *vaddr;
};

struct virtio_vring_info
{
  struct virtqueue       *vq;
  struct vring_alloc_info info;
  uint32_t                notifyid;
};

struct virtio_device
{
  struct virtio_device_id  id;
  void                    *priv;
  struct virtio_vring_info *vrings_info;
  unsigned int             vrings_num;
  virtio_mmio_dev_t        mmio_dev;
  uint32_t                 device_num;
  int                      irq;
};

struct virtio_driver
{
  struct list_node node;
  uint32_t         device;
  int            (*probe)(struct virtio_device *vdev);
  void           (*remove)(struct virtio_device *vdev);
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

int virtio_mmio_probe_driver(uint32_t device_num,
                             struct virtio_driver *driver);
int virtio_register_driver(struct virtio_driver *driver);

void virtio_set_status(struct virtio_device *vdev, uint8_t status);
uint8_t virtio_get_status(struct virtio_device *vdev);
void virtio_reset_device(struct virtio_device *vdev);
void virtio_set_features(struct virtio_device *vdev, uint64_t features);
uint64_t virtio_get_features(struct virtio_device *vdev);

void *virtio_zalloc_buf(struct virtio_device *vdev, size_t size,
                        size_t align);
void virtio_free_buf(struct virtio_device *vdev, void *buf);

int virtio_create_virtqueues(struct virtio_device *vdev, unsigned int flags,
                             unsigned int nvqs, const char *names[],
                             vq_callback callbacks[], void *callback_args[]);
void virtio_delete_virtqueues(struct virtio_device *vdev);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ARCH_ARM_SRC_QEMU_VIRT_VIRTIO_VIRTIO_H */
