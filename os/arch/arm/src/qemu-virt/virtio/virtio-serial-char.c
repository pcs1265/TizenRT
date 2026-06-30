/****************************************************************************
 * drivers/virtio/virtio-serial-char.c
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
#include <fcntl.h>
#include <limits.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <tinyara/fs/fs.h>
#include <tinyara/kmalloc.h>
#include <tinyara/spinlock.h>

#include "virtio.h"
#include "virtio-serial-char.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VIRTIO_SERIAL_CHAR_RX        0
#define VIRTIO_SERIAL_CHAR_TX        1
#define VIRTIO_SERIAL_CHAR_NUM       2

#define VIRTIO_SERIAL_CHAR_NO_CHUNK  (-1)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct virtio_serial_char_chunk_s
{
  FAR char *buffer;
  size_t    len;
  int       next;
};

struct virtio_serial_char_priv_s
{
  FAR struct virtio_device              *vdev;
  char                                   name[NAME_MAX];
  spinlock_t                             lock;
  sem_t                                  free_sem;
  FAR struct virtio_serial_char_chunk_s *chunks;
  int                                    free_head;
  uint16_t                               nchunks;
  size_t                                 chunk_size;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ssize_t virtio_serial_char_write(FAR struct file *filep,
                                        FAR const char *buffer,
                                        size_t buflen);
static void virtio_serial_char_txdone(FAR struct virtqueue *vq);
static int virtio_serial_char_probe(FAR struct virtio_device *vdev);
static void virtio_serial_char_remove(FAR struct virtio_device *vdev);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_virtio_serial_char_fops =
{
  NULL,                         /* open */
  NULL,                         /* close */
  NULL,                         /* read */
  virtio_serial_char_write,     /* write */
  NULL,                         /* seek */
  NULL                          /* ioctl */
#ifndef CONFIG_DISABLE_POLL
  , NULL                        /* poll */
#endif
};

static struct virtio_driver g_virtio_serial_char_driver =
{
  LIST_INITIAL_VALUE(g_virtio_serial_char_driver.node), /* node */
  VIRTIO_ID_CONSOLE,                                    /* device id */
  virtio_serial_char_probe,                             /* probe */
  virtio_serial_char_remove,                            /* remove */
};

static struct virtio_driver g_virtio_rprocserial_char_driver =
{
  LIST_INITIAL_VALUE(g_virtio_rprocserial_char_driver.node), /* node */
  VIRTIO_ID_RPROC_SERIAL,                                    /* device id */
  virtio_serial_char_probe,                                  /* probe */
  virtio_serial_char_remove,                                 /* remove */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void
virtio_serial_char_free_chunk(FAR struct virtio_serial_char_priv_s *priv,
                              FAR struct virtio_serial_char_chunk_s *chunk)
{
  irqstate_t flags;
  int index;

  index = chunk - priv->chunks;

  flags = spin_lock_irqsave(&priv->lock);
  chunk->len = 0;
  chunk->next = priv->free_head;
  priv->free_head = index;
  spin_unlock_irqrestore(&priv->lock, flags);

  sem_post(&priv->free_sem);
}

static int
virtio_serial_char_alloc_chunk(FAR struct virtio_serial_char_priv_s *priv,
                               bool nonblock,
                               FAR struct virtio_serial_char_chunk_s **out)
{
  irqstate_t flags;
  int index;
  int ret;

  if (nonblock)
    {
      ret = sem_trywait(&priv->free_sem);
      if (ret < 0)
        {
          return -errno;
        }
    }
  else
    {
      do
        {
          ret = sem_wait(&priv->free_sem);
        }
      while (ret < 0 && errno == EINTR);

      if (ret < 0)
        {
          return -errno;
        }
    }

  flags = spin_lock_irqsave(&priv->lock);
  index = priv->free_head;
  if (index == VIRTIO_SERIAL_CHAR_NO_CHUNK)
    {
      spin_unlock_irqrestore(&priv->lock, flags);
      return -EAGAIN;
    }

  priv->free_head = priv->chunks[index].next;
  priv->chunks[index].next = VIRTIO_SERIAL_CHAR_NO_CHUNK;
  spin_unlock_irqrestore(&priv->lock, flags);

  *out = &priv->chunks[index];
  return OK;
}

static int
virtio_serial_char_queue_chunk(FAR struct virtio_serial_char_priv_s *priv,
                               FAR struct virtio_serial_char_chunk_s *chunk)
{
  FAR struct virtqueue *vq;
  struct virtqueue_buf vb;
  irqstate_t flags;
  int ret;

  vq = priv->vdev->vrings_info[VIRTIO_SERIAL_CHAR_TX].vq;
  vb.buf = chunk->buffer;
  vb.len = chunk->len;

  flags = spin_lock_irqsave(&priv->lock);
  ret = virtqueue_add_buffer(vq, &vb, 1, 0, chunk);
  if (ret == OK)
    {
      virtqueue_kick(vq);
    }

  spin_unlock_irqrestore(&priv->lock, flags);
  return ret;
}

static ssize_t virtio_serial_char_write(FAR struct file *filep,
                                        FAR const char *buffer,
                                        size_t buflen)
{
  FAR struct virtio_serial_char_priv_s *priv;
  FAR struct virtio_serial_char_chunk_s *chunk;
  FAR struct inode *inode;
  ssize_t nwritten = 0;
  bool nonblock;
  size_t ncopy;
  int ret;

  if (buflen == 0)
    {
      return 0;
    }

  inode = filep->f_inode;
  priv = inode->i_private;
  nonblock = (filep->f_oflags & O_NONBLOCK) != 0;

  while ((size_t)nwritten < buflen)
    {
      ret = virtio_serial_char_alloc_chunk(priv, nonblock, &chunk);
      if (ret < 0)
        {
          return nwritten > 0 ? nwritten : ret;
        }

      ncopy = buflen - nwritten;
      if (ncopy > priv->chunk_size)
        {
          ncopy = priv->chunk_size;
        }

      memcpy(chunk->buffer, &buffer[nwritten], ncopy);
      chunk->len = ncopy;

      ret = virtio_serial_char_queue_chunk(priv, chunk);
      if (ret < 0)
        {
          virtio_serial_char_free_chunk(priv, chunk);
          return nwritten > 0 ? nwritten : ret;
        }

      nwritten += ncopy;
    }

  return nwritten;
}

static void virtio_serial_char_txdone(FAR struct virtqueue *vq)
{
  FAR struct virtio_serial_char_priv_s *priv = vq->vq_dev->priv;
  FAR struct virtio_serial_char_chunk_s *chunk;

  for (;;)
    {
      chunk = virtqueue_get_buffer_lock(vq, NULL, NULL, &priv->lock);
      if (chunk == NULL)
        {
          break;
        }

      virtio_serial_char_free_chunk(priv, chunk);
    }
}

static int
virtio_serial_char_alloc_chunks(FAR struct virtio_serial_char_priv_s *priv)
{
  FAR struct virtqueue *vq;
  uint16_t configured;
  uint16_t i;

  vq = priv->vdev->vrings_info[VIRTIO_SERIAL_CHAR_TX].vq;
  configured = CONFIG_QEMU_VIRT_VIRTIO_SERIAL_CHAR_CHUNKS;
  priv->nchunks = configured < vq->num ? configured : vq->num;
  if (priv->nchunks == 0)
    {
      return -ENODEV;
    }

  priv->chunk_size = CONFIG_QEMU_VIRT_VIRTIO_SERIAL_CHAR_CHUNK_SIZE;
  priv->free_head = VIRTIO_SERIAL_CHAR_NO_CHUNK;
  priv->chunks = kmm_zalloc(sizeof(*priv->chunks) * priv->nchunks);
  if (priv->chunks == NULL)
    {
      return -ENOMEM;
    }

  sem_init(&priv->free_sem, 0, 0);

  for (i = 0; i < priv->nchunks; i++)
    {
      priv->chunks[i].buffer =
        virtio_zalloc_buf(priv->vdev, priv->chunk_size, 16);
      if (priv->chunks[i].buffer == NULL)
        {
          return -ENOMEM;
        }

      priv->chunks[i].next = priv->free_head;
      priv->free_head = i;
      sem_post(&priv->free_sem);
    }

  return OK;
}

static void
virtio_serial_char_free_chunks(FAR struct virtio_serial_char_priv_s *priv)
{
  uint16_t i;

  if (priv->chunks == NULL)
    {
      return;
    }

  for (i = 0; i < priv->nchunks; i++)
    {
      if (priv->chunks[i].buffer != NULL)
        {
          virtio_free_buf(priv->vdev, priv->chunks[i].buffer);
        }
    }

  sem_destroy(&priv->free_sem);
  kmm_free(priv->chunks);
  priv->chunks = NULL;
}

static int
virtio_serial_char_init(FAR struct virtio_serial_char_priv_s *priv,
                        FAR struct virtio_device *vdev)
{
  FAR const char *vqnames[VIRTIO_SERIAL_CHAR_NUM];
  vq_callback callbacks[VIRTIO_SERIAL_CHAR_NUM];
  int ret;

  priv->vdev = vdev;
  vdev->priv = priv;
  spin_initialize(&priv->lock, SP_UNLOCKED);

  virtio_set_status(vdev, VIRTIO_CONFIG_STATUS_DRIVER);
  virtio_set_features(vdev, 0);
  virtio_set_status(vdev, VIRTIO_CONFIG_FEATURES_OK);

  vqnames[VIRTIO_SERIAL_CHAR_RX]   = "virtio_serial_char_rx";
  vqnames[VIRTIO_SERIAL_CHAR_TX]   = "virtio_serial_char_tx";
  callbacks[VIRTIO_SERIAL_CHAR_RX] = NULL;
  callbacks[VIRTIO_SERIAL_CHAR_TX] = virtio_serial_char_txdone;

  ret = virtio_create_virtqueues(vdev, 0, VIRTIO_SERIAL_CHAR_NUM, vqnames,
                                 callbacks, NULL);
  if (ret < 0)
    {
      vrterr("virtio_create_virtqueues failed, ret=%d\n", ret);
      goto err_reset;
    }

  ret = virtio_serial_char_alloc_chunks(priv);
  if (ret < 0)
    {
      vrterr("virtio_serial_char_alloc_chunks failed, ret=%d\n", ret);
      goto err_chunks;
    }

  virtqueue_enable_cb(vdev->vrings_info[VIRTIO_SERIAL_CHAR_TX].vq);
  virtio_set_status(vdev, VIRTIO_CONFIG_STATUS_DRIVER_OK);
  return OK;

err_chunks:
  virtio_serial_char_free_chunks(priv);
err_vq:
  virtio_delete_virtqueues(vdev);
err_reset:
  virtio_reset_device(vdev);
  return ret;
}

static void
virtio_serial_char_uninit(FAR struct virtio_serial_char_priv_s *priv)
{
  virtio_reset_device(priv->vdev);
  virtio_delete_virtqueues(priv->vdev);
  virtio_serial_char_free_chunks(priv);
}

static int
virtio_serial_char_register(FAR struct virtio_serial_char_priv_s *priv)
{
  FAR const char *name = CONFIG_QEMU_VIRT_VIRTIO_SERIAL_CHAR_NAME;

  if (name[0] == '/')
    {
      snprintf(priv->name, NAME_MAX, "%s", name);
    }
  else
    {
      snprintf(priv->name, NAME_MAX, "/dev/%s", name);
    }

  return register_driver(priv->name, &g_virtio_serial_char_fops, 0666, priv);
}

static int virtio_serial_char_probe(FAR struct virtio_device *vdev)
{
  FAR struct virtio_serial_char_priv_s *priv;
  int ret;

  priv = kmm_zalloc(sizeof(*priv));
  if (priv == NULL)
    {
      return -ENOMEM;
    }

  ret = virtio_serial_char_init(priv, vdev);
  if (ret < 0)
    {
      goto err_priv;
    }

  ret = virtio_serial_char_register(priv);
  if (ret < 0)
    {
      goto err_init;
    }

  vrtinfo("%s: chunks=%u chunk_size=%u\n", priv->name, priv->nchunks,
          (unsigned int)priv->chunk_size);
  return OK;

err_init:
  virtio_serial_char_uninit(priv);
err_priv:
  kmm_free(priv);
  return ret;
}

static void virtio_serial_char_remove(FAR struct virtio_device *vdev)
{
  FAR struct virtio_serial_char_priv_s *priv = vdev->priv;

  unregister_driver(priv->name);
  virtio_serial_char_uninit(priv);
  kmm_free(priv);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int virtio_serial_char_driver_initialize(uint32_t device_num)
{
  int ret;

  ret = virtio_mmio_probe_driver(device_num, &g_virtio_serial_char_driver);
  if (ret == -ENODEV)
    {
      ret = virtio_mmio_probe_driver(device_num,
                                     &g_virtio_rprocserial_char_driver);
    }

  return ret;
}
