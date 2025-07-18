/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 *
 *   Copyright (C) 2007-2008, 2011, 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __INCLUDE_KMALLOC_H
#define __INCLUDE_KMALLOC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>

#include <tinyara/mm/mm.h>
#include <tinyara/userspace.h>

#if !defined(CONFIG_BUILD_PROTECTED) || defined(__KERNEL__)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef KMALLOC_EXTERN
#if defined(__cplusplus)
#define KMALLOC_EXTERN extern "C"
extern "C" {
#else
#define KMALLOC_EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* For a monolithic, kernel-mode TinyAra build.  Special allocators must be
 * used.  Otherwise, the standard allocators prototyped in stdlib.h may
 * be used for both the kernel- and user-mode objects.
 */

/* This family of allocators is used to manage user-accessible memory
 * from the kernel.  In the flat build, the following are declared in
 * stdlib.h and are directly callable.  In the kernel-phase of the kernel
 * build, the following are defined in userspace.h as macros that call
 * into user-space via a header at the beginning of the user-space blob.
 */

#define kumm_initialize(h, s)     umm_initialize(h, s)
#define kumm_addregion(h, s)      umm_addregion(h, s)
#define kumm_trysemaphore(a)      umm_trysemaphore(a)
#define kumm_givesemaphore(a)     umm_givesemaphore(a)

#ifdef CONFIG_KMM_FORCE_ALLOC_AT
#define kumm_malloc(s)          malloc_at(CONFIG_KMM_FORCE_ALLOC_IDX, s)
#define kumm_zalloc(s)          zalloc_at(CONFIG_KMM_FORCE_ALLOC_IDX, s)
#define kumm_realloc(p, s)      realloc_at(CONFIG_KMM_FORCE_ALLOC_IDX, p, s)
#else
#define kumm_malloc(s)          malloc(s)
#define kumm_zalloc(s)          zalloc(s)
#define kumm_realloc(p, s)      realloc(p, s)
#endif
#define kumm_memalign(a, s)     memalign(a, s)
#define kumm_free(p)            free(p)
#define kumm_mallinfo()         mallinfo()

/* This family of allocators is used to manage kernel protected memory */

#if !defined(CONFIG_MM_KERNEL_HEAP)
/* If this the kernel phase of a kernel build, and there are only user-space
 * allocators, then the following are defined in userspace.h as macros that
 * call into user-space via a header at the beginning of the user-space blob.
 */

#define kmm_initialize(h, s)		/* Initialization done by kumm_initialize */
#define kmm_addregion(h, s)     umm_addregion(h, s)
#define kmm_trysemaphore(a)     umm_trysemaphore(a)
#define kmm_givesemaphore(a)    umm_givesemaphore(a)

#define kmm_malloc(s)          umm_malloc(s)
#define kmm_zalloc(s)          umm_zalloc(s)
#define kmm_realloc(p, s)      umm_realloc(p, s)

#define kmm_memalign(a, s)     umm_memalign(a, s)
#define kmm_free(p)            umm_free(p)
#define kmm_mallinfo()         umm_mallinfo()

#else
/* Otherwise, the kernel-space allocators are declared in include/tinyara/mm/mm.h
 * and we can call them directly.
 */

#endif

#if (defined(CONFIG_BUILD_PROTECTED) || defined(CONFIG_BUILD_KERNEL)) && \
	 defined(CONFIG_MM_KERNEL_HEAP)
/****************************************************************************
 * Group memory management
 *
 *   Manage memory allocations appropriately for the group type.  If the
 *   memory is part of a privileged group, then it should be allocated so
 *   that it is only accessible by privileged code;  Otherwise, it is a
 *   user mode group and must be allocated so that it accessible by
 *   unprivileged code.
 *
 ****************************************************************************/
/* Functions defined in group/group_malloc.c ********************************/

FAR void *group_malloc(FAR struct task_group_s *group, size_t nbytes);

/* Functions defined in group/group_zalloc.c ********************************/

FAR void *group_zalloc(FAR struct task_group_s *group, size_t nbytes);

/* Functions defined in group/group_free.c **********************************/

void group_free(FAR struct task_group_s *group, FAR void *mem);

#else
/* In the flat build, there is only one memory allocator and no distinction
 * in privileges.
 */

#define group_malloc(g, n) kumm_malloc(n)
#define group_zalloc(g, n) kumm_zalloc(n)
#define group_free(g, m)   kumm_free(m)

#endif

/* Functions defined in sched/sched_kfree.c **********************************/

/* Handles memory freed from an interrupt handler.  In that context, kmm_free()
 * (or kumm_free()) cannot be called.  Instead, the allocations are saved in a
 * list of delayed allocations that will be periodically cleaned up by
 * sched_garbagecollection().
 */

void sched_ufree(FAR void *address);

#if (defined(CONFIG_BUILD_PROTECTED) || defined(CONFIG_BUILD_KERNEL)) && \
	 defined(CONFIG_MM_KERNEL_HEAP) && defined(__KERNEL__)
void sched_kfree(FAR void *address);
#else
#define sched_kfree(a) sched_ufree(a)
#endif


#undef KMALLOC_EXTERN
#if defined(__cplusplus)
}
#endif

#endif							/* !CONFIG_BUILD_PROTECTED || __KERNEL__ */
#endif							/* __INCLUDE_KMALLOC_H */
