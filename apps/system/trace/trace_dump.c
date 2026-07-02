/****************************************************************************
 * apps/system/trace/trace_dump.c
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
#include <tinyara/note/noteram_driver.h>

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <errno.h>

#include "trace.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TRACE_RAW_MAGIC "TZNOTE01"
#define TRACE_RAW_MAGIC_SIZE 8

/****************************************************************************
 * Name: note_ioctl
 ****************************************************************************/

static void note_ioctl(int cmd, unsigned long arg)
{
  int notefd;

  notefd = open("/dev/note/ram", O_RDONLY);
  if (notefd < 0)
    {
      fprintf(stderr, "trace: cannot open /dev/note/ram\n");
      return;
    }

  ioctl(notefd, cmd, arg);
  close(notefd);
}

static int trace_write_u16(FAR FILE *out, uint16_t value)
{
  uint8_t data[2];

  data[0] = value;
  data[1] = value >> 8;
  return fwrite(data, 1, sizeof(data), out) == sizeof(data) ? OK : ERROR;
}

static int trace_write_u32(FAR FILE *out, uint32_t value)
{
  uint8_t data[4];

  data[0] = value;
  data[1] = value >> 8;
  data[2] = value >> 16;
  data[3] = value >> 24;
  return fwrite(data, 1, sizeof(data), out) == sizeof(data) ? OK : ERROR;
}

static int trace_write_binary_header(FAR FILE *out, int fd)
{
  FAR struct noteram_taskname_list_s *list;
  size_t namelen;
  int ret = ERROR;
  int i;

  list = calloc(1, sizeof(*list));
  if (list == NULL)
    {
      fprintf(stderr, "trace: cannot allocate task-name snapshot\n");
      return ERROR;
    }

  list->capacity = NOTERAM_TASKNAME_MAX;
  if (ioctl(fd, NOTERAM_GETTASKNAMES, (unsigned long)list) < 0)
    {
      fprintf(stderr, "trace: cannot get task-name snapshot: %d\n", errno);
      goto out;
    }

  if (fwrite(TRACE_RAW_MAGIC, 1, TRACE_RAW_MAGIC_SIZE, out) !=
      TRACE_RAW_MAGIC_SIZE ||
      trace_write_u32(out, list->frequency) < 0 ||
      trace_write_u16(out, list->count) < 0 ||
      trace_write_u16(out, NOTERAM_TASKNAME_SIZE) < 0)
    {
      goto out;
    }

  for (i = 0; i < list->count; i++)
    {
      namelen = strnlen(list->entries[i].name, NOTERAM_TASKNAME_SIZE);
      if (trace_write_u16(out, (uint16_t)list->entries[i].pid) < 0 ||
          fwrite(list->entries[i].name, 1, namelen, out) != namelen)
        {
          goto out;
        }

      while (namelen++ < NOTERAM_TASKNAME_SIZE)
        {
          if (fputc('\0', out) == EOF)
            {
              goto out;
            }
        }
    }

  ret = OK;

out:
  free(list);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trace_dump
 *
 * Description:
 *   Read notes and dump trace results.
 *
 ****************************************************************************/

int trace_dump(FAR FILE *out, bool binary)
{
  uint8_t tracedata[1024];
  unsigned int readmode;
  int ret;
  int fd;

  /* Open note for read */

  fd = open("/dev/note/ram", O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "trace: cannot open /dev/note/ram\n");
      return ERROR;
    }

  /* Select whether the driver returns formatted text or raw note bytes. */

  readmode = binary ? NOTERAM_MODE_READ_BINARY : NOTERAM_MODE_READ_ASCII;
  ret = ioctl(fd, NOTERAM_SETREADMODE, (unsigned long)&readmode);
  if (ret < 0)
    {
      fprintf(stderr, "trace: cannot set note read mode: %d\n", errno);
      close(fd);
      return ERROR;
    }

  if (binary && trace_write_binary_header(out, fd) < 0)
    {
      fprintf(stderr, "trace: cannot write binary trace header\n");
      close(fd);
      return ERROR;
    }

  /* Read and output all notes */

  while (1)
    {
      ret = read(fd, tracedata, sizeof tracedata);
      if (ret < 0 || ret > sizeof(tracedata))
        {
          fprintf(stderr, "trace: read error: %d, errno:%d\n", ret, errno);
          continue;
        }
      else if (ret == 0)
        {
          break;
        }

      fwrite(tracedata, 1, ret, out);
    }

  /* Close note */

  close(fd);

  return ret;
}

/****************************************************************************
 * Name: trace_dump_clear
 *
 * Description:
 *   Clear all contents of the buffer
 *
 ****************************************************************************/

void trace_dump_clear(void)
{
  note_ioctl(NOTERAM_CLEAR, 0);
}

/****************************************************************************
 * Name: trace_dump_get_overwrite
 *
 * Description:
 *   Get overwrite mode
 *
 ****************************************************************************/

bool trace_dump_get_overwrite(void)
{
  unsigned int mode = 0;

  note_ioctl(NOTERAM_GETMODE, (unsigned long)&mode);

  return mode == NOTERAM_MODE_OVERWRITE_ENABLE;
}

/****************************************************************************
 * Name: trace_dump_set_overwrite
 *
 * Description:
 *   Set overwrite mode
 *
 ****************************************************************************/

void trace_dump_set_overwrite(bool enable)
{
  unsigned int mode;

  mode = enable ? NOTERAM_MODE_OVERWRITE_ENABLE :
                  NOTERAM_MODE_OVERWRITE_DISABLE;

  note_ioctl(NOTERAM_SETMODE, (unsigned long)&mode);
}
