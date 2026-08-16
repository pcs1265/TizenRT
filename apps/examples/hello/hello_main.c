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
 * examples/hello/hello_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define KSC001_TIMEOUT_SECONDS 2
#define KSC002_TIMEOUT_SECONDS 2
#define KSC002_WORKER_COUNT 2
#define KSC002_INCREMENTS_PER_WORKER 32
#define KSC003_TIMEOUT_SECONDS 2
#define KSC004_TIMEOUT_SECONDS 2
#define KSC005_TIMEOUT_SECONDS 2
#define KSC005_WORKER_COUNT 2
#define KSC006_TIMEOUT_SECONDS 2
#define KSC006_WORKER_COUNT 2
#define KSC007_TIMEOUT_SECONDS 2
#define KSC007_WORKER_COUNT 2
#define KSC008_TIMEOUT_SECONDS 2
#define KSC009_TIMEOUT_SECONDS 2
#define KSC009_WORKER_COUNT 2

#ifndef CONFIG_SMP_NCPUS
#define CONFIG_SMP_NCPUS 1
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t g_ksc001_ready;
static char g_ksc001_exit_token;
static sem_t g_ksc002_start;
static sem_t g_ksc002_done;
static pthread_mutex_t g_ksc002_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_ksc002_counter;
static pthread_mutex_t g_ksc003_lock;
static pthread_cond_t g_ksc003_ready;
static int g_ksc003_predicate;
static sem_t g_ksc004_ready;
static pthread_key_t g_ksc004_key;
static int g_ksc004_value;
static sem_t g_ksc005_done;
static pthread_once_t g_ksc005_once = PTHREAD_ONCE_INIT;
static int g_ksc005_init_count;
static pthread_rwlock_t g_ksc006_lock;
static sem_t g_ksc006_ready;
static sem_t g_ksc006_release;
static int g_ksc006_exit_token;
static pthread_barrier_t g_ksc007_barrier;
static sem_t g_ksc007_start;
static sem_t g_ksc007_done;
static int g_ksc007_barrier_result[KSC007_WORKER_COUNT];
static int g_ksc007_exit_token;
static pthread_mutex_t g_ksc008_lock;
static sem_t g_ksc008_done;
static int g_ksc008_trylock_status;
static int g_ksc008_exit_token;
static pthread_mutex_t g_ksc009_lock;
static pthread_cond_t g_ksc009_condition;
static sem_t g_ksc009_waiting;
static sem_t g_ksc009_done;
static int g_ksc009_predicate;
static int g_ksc009_woken;
static int g_ksc009_exit_token;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* KSC-001 worker: publish that it ran, then exercise normal pthread exit. */

static pthread_addr_t ksc001_worker(pthread_addr_t arg)
{
	(void)arg;

	if (sem_post(&g_ksc001_ready) != 0) {
		return NULL;
	}

	return &g_ksc001_exit_token;
}

/* KSC-001: scheduler/task lifecycle.
 *
 * Purpose: Verify a created task runs, wakes its creator, returns a value,
 * and is reaped by pthread_join().  sem_timedwait() bounds the observation;
 * a timeout triggers cancellation and join cleanup before the semaphore is
 * destroyed.
 */

static int ksc001_task_lifecycle(void)
{
	struct timespec deadline;
	pthread_t worker;
	pthread_addr_t result = NULL;
	int status;
	int failed = 0;

	printf("KSC-001: START task lifecycle (timeout=%d s)\n",
	       KSC001_TIMEOUT_SECONDS);

	if (sem_init(&g_ksc001_ready, 0, 0) != 0) {
		printf("KSC-001: FAIL sem_init errno=%d\n", errno);
		return -1;
	}

	status = pthread_create(&worker, NULL, ksc001_worker, NULL);
	if (status != 0) {
		printf("KSC-001: FAIL pthread_create status=%d\n", status);
		(void)sem_destroy(&g_ksc001_ready);
		return -1;
	}

	if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
		printf("KSC-001: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	} else {
		deadline.tv_sec += KSC001_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc001_ready, &deadline) != 0) {
			printf("KSC-001: FAIL worker notification errno=%d\n", errno);
			failed = 1;
		}
	}

	if (failed) {
		/* A non-notifying worker must not be left behind by this test. */
		status = pthread_cancel(worker);
		if (status != 0) {
			printf("KSC-001: cleanup pthread_cancel status=%d\n", status);
		}
	}

	status = pthread_join(worker, &result);
	if (status != 0) {
		printf("KSC-001: FAIL pthread_join status=%d\n", status);
		failed = 1;
	} else if (!failed && result != &g_ksc001_exit_token) {
		printf("KSC-001: FAIL exit value=%p\n", result);
		failed = 1;
	}

	if (sem_destroy(&g_ksc001_ready) != 0) {
		printf("KSC-001: FAIL sem_destroy errno=%d\n", errno);
		failed = 1;
	}

	printf("KSC-001: %s task create -> wake -> exit -> join\n",
	       failed ? "FAIL" : "PASS");
	return failed ? -1 : 0;
}

/* KSC-002 worker: wait for a common start, then contend on the mutex. */

static pthread_addr_t ksc002_worker(pthread_addr_t arg)
{
	int i;

	(void)arg;
	if (sem_wait(&g_ksc002_start) != 0) {
		return NULL;
	}

	for (i = 0; i < KSC002_INCREMENTS_PER_WORKER; i++) {
		if (pthread_mutex_lock(&g_ksc002_lock) != 0) {
			return NULL;
		}
		g_ksc002_counter++;
		if (pthread_mutex_unlock(&g_ksc002_lock) != 0) {
			return NULL;
		}
	}

	(void)sem_post(&g_ksc002_done);
	return NULL;
}

/* KSC-002: mutex contention and ownership handoff.
 *
 * Two workers are released together and repeatedly update one protected
 * counter. Every completion observation is time bounded; the failure path
 * cancels and joins both workers before destroying synchronization state.
 */

static int ksc002_mutex_contention(void)
{
	struct timespec deadline;
	pthread_t workers[KSC002_WORKER_COUNT];
	int created = 0;
	int completed = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-002: START mutex contention (timeout=%d s)\n",
	       KSC002_TIMEOUT_SECONDS);
	g_ksc002_counter = 0;
	if (sem_init(&g_ksc002_start, 0, 0) != 0) {
		printf("KSC-002: FAIL start sem_init errno=%d\n", errno);
		return -1;
	}
	if (sem_init(&g_ksc002_done, 0, 0) != 0) {
		printf("KSC-002: FAIL done sem_init errno=%d\n", errno);
		(void)sem_destroy(&g_ksc002_start);
		return -1;
	}

	for (i = 0; i < KSC002_WORKER_COUNT; i++) {
		status = pthread_create(&workers[i], NULL, ksc002_worker, NULL);
		if (status != 0) {
			printf("KSC-002: FAIL pthread_create[%d] status=%d\n", i, status);
			failed = 1;
			break;
		}
		created++;
	}
	for (i = 0; i < created; i++) {
		if (sem_post(&g_ksc002_start) != 0) {
			printf("KSC-002: FAIL sem_post errno=%d\n", errno);
			failed = 1;
		}
	}

	if (!failed) {
		if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
			printf("KSC-002: FAIL clock_gettime errno=%d\n", errno);
			failed = 1;
		} else {
			deadline.tv_sec += KSC002_TIMEOUT_SECONDS;
			while (!failed && completed < created) {
				if (sem_timedwait(&g_ksc002_done, &deadline) != 0) {
					printf("KSC-002: FAIL worker completion errno=%d\n", errno);
					failed = 1;
				} else {
					completed++;
				}
			}
		}
	}
	if (created != KSC002_WORKER_COUNT) {
		failed = 1;
	}
	if (failed) {
		for (i = 0; i < created; i++) {
			(void)pthread_cancel(workers[i]);
		}
	}
	for (i = 0; i < created; i++) {
		status = pthread_join(workers[i], NULL);
		if (status != 0) {
			printf("KSC-002: FAIL pthread_join[%d] status=%d\n", i, status);
			failed = 1;
		}
	}
	if (!failed && g_ksc002_counter !=
	    KSC002_WORKER_COUNT * KSC002_INCREMENTS_PER_WORKER) {
		printf("KSC-002: FAIL counter=%d expected=%d\n", g_ksc002_counter,
		       KSC002_WORKER_COUNT * KSC002_INCREMENTS_PER_WORKER);
		failed = 1;
	}
	if (sem_destroy(&g_ksc002_done) != 0 || sem_destroy(&g_ksc002_start) != 0) {
		printf("KSC-002: FAIL sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	printf("KSC-002: %s mutex handoff counter=%d\n",
	       failed ? "FAIL" : "PASS", g_ksc002_counter);
	return failed ? -1 : 0;
}

/* KSC-003 worker: update the predicate while holding its associated lock. */

static pthread_addr_t ksc003_worker(pthread_addr_t arg)
{
	int status;

	(void)arg;
	status = pthread_mutex_lock(&g_ksc003_lock);
	if (status != 0) {
		return NULL;
	}
	g_ksc003_predicate = 1;
	status = pthread_cond_signal(&g_ksc003_ready);
	if (pthread_mutex_unlock(&g_ksc003_lock) != 0) {
		return NULL;
	}
	return status == 0 ? &g_ksc003_predicate : NULL;
}

/* KSC-003: condition-variable predicate wakeup.  The waiter checks its
 * predicate under the mutex with a finite absolute timeout.  On failure, its
 * worker is cancelled and joined before the synchronization state is removed.
 */

static int ksc003_condition_wakeup(void)
{
	struct timespec deadline;
	pthread_t worker;
	pthread_addr_t result = NULL;
	int created = 0;
	int locked = 0;
	int status;
	int failed = 0;

	printf("KSC-003: START condition predicate wakeup (timeout=%d s)\n",
	       KSC003_TIMEOUT_SECONDS);
	g_ksc003_predicate = 0;
	status = pthread_mutex_init(&g_ksc003_lock, NULL);
	if (status != 0) {
		printf("KSC-003: FAIL pthread_mutex_init status=%d\n", status);
		return -1;
	}
	status = pthread_cond_init(&g_ksc003_ready, NULL);
	if (status != 0) {
		printf("KSC-003: FAIL pthread_cond_init status=%d\n", status);
		(void)pthread_mutex_destroy(&g_ksc003_lock);
		return -1;
	}
	status = pthread_create(&worker, NULL, ksc003_worker, NULL);
	if (status != 0) {
		printf("KSC-003: FAIL pthread_create status=%d\n", status);
		failed = 1;
	} else {
		created = 1;
		status = pthread_mutex_lock(&g_ksc003_lock);
		if (status != 0) {
			printf("KSC-003: FAIL pthread_mutex_lock status=%d\n", status);
			failed = 1;
		} else {
			locked = 1;
			if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
				printf("KSC-003: FAIL clock_gettime errno=%d\n", errno);
				failed = 1;
			} else {
				deadline.tv_sec += KSC003_TIMEOUT_SECONDS;
				while (!failed && !g_ksc003_predicate) {
					status = pthread_cond_timedwait(&g_ksc003_ready,
											&g_ksc003_lock, &deadline);
					if (status != 0) {
						printf("KSC-003: FAIL pthread_cond_timedwait status=%d\n",
						       status);
						failed = 1;
					}
				}
			}
			if (pthread_mutex_unlock(&g_ksc003_lock) != 0) {
				printf("KSC-003: FAIL pthread_mutex_unlock\n");
				failed = 1;
			}
			locked = 0;
		}
	}
	if (locked) {
		(void)pthread_mutex_unlock(&g_ksc003_lock);
	}
	if (failed && created) {
		status = pthread_cancel(worker);
		if (status != 0) {
			printf("KSC-003: cleanup pthread_cancel status=%d\n", status);
		}
	}
	if (created) {
		status = pthread_join(worker, &result);
		if (status != 0) {
			printf("KSC-003: FAIL pthread_join status=%d\n", status);
			failed = 1;
		} else if (!failed && result != &g_ksc003_predicate) {
			printf("KSC-003: FAIL worker result=%p\n", result);
			failed = 1;
		}
	}
	if (pthread_cond_destroy(&g_ksc003_ready) != 0 ||
	    pthread_mutex_destroy(&g_ksc003_lock) != 0) {
		printf("KSC-003: FAIL synchronization destroy\n");
		failed = 1;
	}
	printf("KSC-003: %s condition predicate=%d\n",
	       failed ? "FAIL" : "PASS", g_ksc003_predicate);
	return failed ? -1 : 0;
}

/* KSC-004: thread-specific data is set and read by a worker with an
 * all-active-CPU affinity.  Completion is bounded and every created object is
 * joined or destroyed on the failure path. */
static pthread_addr_t ksc004_worker(pthread_addr_t arg)
{
	(void)arg;
	if (pthread_setspecific(g_ksc004_key, &g_ksc004_value) != 0 ||
		pthread_getspecific(g_ksc004_key) != &g_ksc004_value ||
		sem_post(&g_ksc004_ready) != 0) {
		return NULL;
	}
	return &g_ksc004_value;
}

static int ksc004_thread_specific_data(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t worker;
	pthread_addr_t result = NULL;
	cpu_set_t mask = 0;
	int attr_ready = 0;
	int created = 0;
	int key_ready = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-004: START thread-specific data (timeout=%d s)\n",
	       KSC004_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-004: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	if (sem_init(&g_ksc004_ready, 0, 0) != 0 ||
		pthread_key_create(&g_ksc004_key, NULL) != 0) {
		printf("KSC-004: FAIL setup errno=%d\n", errno);
		if (sem_destroy(&g_ksc004_ready) != 0) { }
		return -1;
	}
	key_ready = 1;
	status = pthread_attr_init(&attr);
	if (status != 0) {
		printf("KSC-004: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	} else {
		attr_ready = 1;
		status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-004: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	}
	if (!failed && (status = pthread_create(&worker, &attr, ksc004_worker, NULL)) == 0) {
		created = 1;
	} else if (!failed) {
		printf("KSC-004: FAIL pthread_create status=%d\n", status);
		failed = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC004_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc004_ready, &deadline) != 0) {
			printf("KSC-004: FAIL worker notification errno=%d\n", errno);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-004: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (failed && created) {
		(void)pthread_cancel(worker);
	}
	if (created && ((status = pthread_join(worker, &result)) != 0 ||
		(!failed && result != &g_ksc004_value))) {
		printf("KSC-004: FAIL pthread_join status=%d result=%p\n", status, result);
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		failed = 1;
	}
	if (key_ready && pthread_key_delete(g_ksc004_key) != 0) {
		failed = 1;
	}
	if (sem_destroy(&g_ksc004_ready) != 0) {
		failed = 1;
	}
	printf("KSC-004: %s thread-specific value=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", g_ksc004_value, (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-005: concurrent pthread_once callers must observe exactly one completed
 * initializer. Both workers use the all-active-CPU affinity mask, and the
 * creator bounds their completions before joining them. */
static void ksc005_initializer(void)
{
	g_ksc005_init_count++;
}

static pthread_addr_t ksc005_worker(pthread_addr_t arg)
{
	int status;

	(void)arg;
	status = pthread_once(&g_ksc005_once, ksc005_initializer);
	if (status != 0 || g_ksc005_init_count != 1 ||
		sem_post(&g_ksc005_done) != 0) {
		return NULL;
	}
	return &g_ksc005_init_count;
}

static int ksc005_once_concurrency(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t workers[KSC005_WORKER_COUNT];
	pthread_addr_t result;
	cpu_set_t mask = 0;
	int attr_ready = 0;
	int created = 0;
	int completed = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-005: START pthread_once concurrency (timeout=%d s)\n",
	       KSC005_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-005: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	if (sem_init(&g_ksc005_done, 0, 0) != 0) {
		printf("KSC-005: FAIL sem_init errno=%d\n", errno);
		return -1;
	}
	status = pthread_attr_init(&attr);
	if (status != 0) {
		printf("KSC-005: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	} else {
		attr_ready = 1;
		status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-005: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	}
	for (i = 0; !failed && i < KSC005_WORKER_COUNT; i++) {
		status = pthread_create(&workers[i], &attr, ksc005_worker, NULL);
		if (status != 0) {
			printf("KSC-005: FAIL pthread_create[%d] status=%d\n", i, status);
			failed = 1;
			break;
		}
		created++;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC005_TIMEOUT_SECONDS;
		while (!failed && completed < KSC005_WORKER_COUNT) {
			if (sem_timedwait(&g_ksc005_done, &deadline) != 0) {
				printf("KSC-005: FAIL worker completion errno=%d\n", errno);
				failed = 1;
			} else {
				completed++;
			}
		}
	} else if (!failed) {
		printf("KSC-005: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (created != KSC005_WORKER_COUNT) {
		failed = 1;
	}
	if (failed) {
		for (i = 0; i < created; i++) {
			(void)pthread_cancel(workers[i]);
		}
	}
	for (i = 0; i < created; i++) {
		result = NULL;
		status = pthread_join(workers[i], &result);
		if (status != 0 || (!failed && result != &g_ksc005_init_count)) {
			printf("KSC-005: FAIL pthread_join[%d] status=%d result=%p\n",
			       i, status, result);
			failed = 1;
		}
	}
	if (g_ksc005_init_count != 1) {
		printf("KSC-005: FAIL initializer count=%d\n", g_ksc005_init_count);
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-005: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if (sem_destroy(&g_ksc005_done) != 0) {
		printf("KSC-005: FAIL sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	printf("KSC-005: %s pthread_once initializer count=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", g_ksc005_init_count,
	       (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-006: two readers must both acquire a read lock before either is allowed
 * to release it. This tests reader sharing with bounded observation and joins. */
static pthread_addr_t ksc006_reader(pthread_addr_t arg)
{
	int status;

	(void)arg;
	status = pthread_rwlock_rdlock(&g_ksc006_lock);
	if (status != 0) {
		return NULL;
	}
	if (sem_post(&g_ksc006_ready) != 0) {
		(void)pthread_rwlock_unlock(&g_ksc006_lock);
		return NULL;
	}
	if (sem_wait(&g_ksc006_release) != 0) {
		(void)pthread_rwlock_unlock(&g_ksc006_lock);
		return NULL;
	}
	if (pthread_rwlock_unlock(&g_ksc006_lock) != 0) {
		return NULL;
	}
	return &g_ksc006_exit_token;
}

static int ksc006_rwlock_reader_sharing(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t workers[KSC006_WORKER_COUNT];
	pthread_addr_t result;
	cpu_set_t mask = 0;
	int attr_ready = 0;
	int created = 0;
	int ready = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-006: START rwlock reader sharing (timeout=%d s)\n",
	       KSC006_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-006: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	status = pthread_rwlock_init(&g_ksc006_lock, NULL);
	if (status != 0) {
		printf("KSC-006: FAIL pthread_rwlock_init status=%d\n", status);
		return -1;
	}
	if (sem_init(&g_ksc006_ready, 0, 0) != 0) {
		printf("KSC-006: FAIL ready sem_init errno=%d\n", errno);
		(void)pthread_rwlock_destroy(&g_ksc006_lock);
		return -1;
	}
	if (sem_init(&g_ksc006_release, 0, 0) != 0) {
		printf("KSC-006: FAIL release sem_init errno=%d\n", errno);
		(void)sem_destroy(&g_ksc006_ready);
		(void)pthread_rwlock_destroy(&g_ksc006_lock);
		return -1;
	}
	status = pthread_attr_init(&attr);
	if (status != 0) {
		printf("KSC-006: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	} else {
		attr_ready = 1;
		status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-006: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	}
	for (i = 0; !failed && i < KSC006_WORKER_COUNT; i++) {
		status = pthread_create(&workers[i], &attr, ksc006_reader, NULL);
		if (status != 0) {
			printf("KSC-006: FAIL pthread_create[%d] status=%d\n", i, status);
			failed = 1;
			break;
		}
		created++;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC006_TIMEOUT_SECONDS;
		while (!failed && ready < KSC006_WORKER_COUNT) {
			if (sem_timedwait(&g_ksc006_ready, &deadline) != 0) {
				printf("KSC-006: FAIL reader acquisition errno=%d\n", errno);
				failed = 1;
			} else {
				ready++;
			}
		}
	} else if (!failed) {
		printf("KSC-006: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (created != KSC006_WORKER_COUNT) {
		failed = 1;
	}
	/* Every created reader gets a release token even on timeout. */
	for (i = 0; i < created; i++) {
		if (sem_post(&g_ksc006_release) != 0) {
			printf("KSC-006: FAIL release sem_post errno=%d\n", errno);
			failed = 1;
		}
	}
	for (i = 0; i < created; i++) {
		result = NULL;
		status = pthread_join(workers[i], &result);
		if (status != 0 || (!failed && result != &g_ksc006_exit_token)) {
			printf("KSC-006: FAIL pthread_join[%d] status=%d result=%p\n",
			       i, status, result);
			failed = 1;
		}
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-006: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if (sem_destroy(&g_ksc006_release) != 0 ||
		sem_destroy(&g_ksc006_ready) != 0 ||
		pthread_rwlock_destroy(&g_ksc006_lock) != 0) {
		printf("KSC-006: FAIL cleanup\n");
		failed = 1;
	}
	printf("KSC-006: %s rwlock concurrent readers=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", ready, (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-007: two all-active-CPU-affined workers meet at a barrier. Exactly one
 * returns PTHREAD_BARRIER_SERIAL_THREAD. A start gate prevents a partial
 * group entering the barrier; all observed completion waits are time bounded. */
static pthread_addr_t ksc007_barrier_worker(pthread_addr_t arg)
{
	int index = (int)(long)arg;
	int status;

	if (sem_wait(&g_ksc007_start) != 0) {
		return NULL;
	}
	status = pthread_barrier_wait(&g_ksc007_barrier);
	g_ksc007_barrier_result[index] = status;
	if ((status != 0 && status != PTHREAD_BARRIER_SERIAL_THREAD) ||
		sem_post(&g_ksc007_done) != 0) {
		return NULL;
	}
	return &g_ksc007_exit_token;
}

static int ksc007_barrier_serial_thread(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t workers[KSC007_WORKER_COUNT];
	pthread_addr_t result;
	cpu_set_t mask = 0;
	int attr_ready = 0;
	int created = 0;
	int completed = 0;
	int serial = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-007: START barrier serial election (timeout=%d s)\n",
	       KSC007_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-007: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	if (sem_init(&g_ksc007_start, 0, 0) != 0 ||
		sem_init(&g_ksc007_done, 0, 0) != 0) {
		printf("KSC-007: FAIL sem_init errno=%d\n", errno);
		(void)sem_destroy(&g_ksc007_start);
		return -1;
	}
	status = pthread_barrier_init(&g_ksc007_barrier, NULL,
				      KSC007_WORKER_COUNT);
	if (status != 0) {
		printf("KSC-007: FAIL pthread_barrier_init status=%d\n", status);
		(void)sem_destroy(&g_ksc007_done);
		(void)sem_destroy(&g_ksc007_start);
		return -1;
	}
	status = pthread_attr_init(&attr);
	if (status != 0) {
		printf("KSC-007: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	} else {
		attr_ready = 1;
		status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-007: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	}
	for (i = 0; !failed && i < KSC007_WORKER_COUNT; i++) {
		g_ksc007_barrier_result[i] = -1;
		status = pthread_create(&workers[i], &attr, ksc007_barrier_worker,
					(FAR void *)(long)i);
		if (status != 0) {
			printf("KSC-007: FAIL pthread_create[%d] status=%d\n", i, status);
			failed = 1;
			break;
		}
		created++;
	}
	if (!failed) {
		for (i = 0; i < created; i++) {
			if (sem_post(&g_ksc007_start) != 0) {
				printf("KSC-007: FAIL start sem_post errno=%d\n", errno);
				failed = 1;
			}
		}
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC007_TIMEOUT_SECONDS;
		while (!failed && completed < KSC007_WORKER_COUNT) {
			if (sem_timedwait(&g_ksc007_done, &deadline) != 0) {
				printf("KSC-007: FAIL barrier completion errno=%d\n", errno);
				failed = 1;
			} else {
				completed++;
			}
		}
	} else if (!failed) {
		printf("KSC-007: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (created != KSC007_WORKER_COUNT) {
		failed = 1;
	}
	if (failed) {
		for (i = 0; i < created; i++) {
			(void)pthread_cancel(workers[i]);
		}
	}
	for (i = 0; i < created; i++) {
		result = NULL;
		status = pthread_join(workers[i], &result);
		if (status != 0 || (!failed && result != &g_ksc007_exit_token)) {
			printf("KSC-007: FAIL pthread_join[%d] status=%d result=%p\n",
			       i, status, result);
			failed = 1;
		}
	}
	for (i = 0; i < KSC007_WORKER_COUNT; i++) {
		if (g_ksc007_barrier_result[i] == PTHREAD_BARRIER_SERIAL_THREAD) {
			serial++;
		} else if (g_ksc007_barrier_result[i] != 0) {
			printf("KSC-007: FAIL barrier result[%d]=%d\n", i,
			       g_ksc007_barrier_result[i]);
			failed = 1;
		}
	}
	if (serial != 1) {
		printf("KSC-007: FAIL serial count=%d\n", serial);
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-007: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if (pthread_barrier_destroy(&g_ksc007_barrier) != 0 ||
		sem_destroy(&g_ksc007_done) != 0 || sem_destroy(&g_ksc007_start) != 0) {
		printf("KSC-007: FAIL cleanup\n");
		failed = 1;
	}
	printf("KSC-007: %s barrier serial count=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", serial, (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-008: a worker must observe EBUSY when it tries to take a mutex held by
 * its creator. The worker has all-active-CPU affinity and reports its result
 * through a bounded semaphore wait before the creator releases the mutex. */
static pthread_addr_t ksc008_trylock_worker(pthread_addr_t arg)
{
	(void)arg;
	g_ksc008_trylock_status = pthread_mutex_trylock(&g_ksc008_lock);
	if (g_ksc008_trylock_status == 0) {
		(void)pthread_mutex_unlock(&g_ksc008_lock);
	}
	if (sem_post(&g_ksc008_done) != 0) {
		return NULL;
	}
	return &g_ksc008_exit_token;
}

static int ksc008_mutex_trylock_busy(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t worker;
	pthread_addr_t result = NULL;
	cpu_set_t mask = 0;
	int attr_ready = 0;
	int created = 0;
	int locked = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-008: START mutex trylock exclusion (timeout=%d s)\n",
	       KSC008_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-008: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	g_ksc008_trylock_status = -1;
	status = pthread_mutex_init(&g_ksc008_lock, NULL);
	if (status != 0) {
		printf("KSC-008: FAIL pthread_mutex_init status=%d\n", status);
		return -1;
	}
	if (sem_init(&g_ksc008_done, 0, 0) != 0) {
		printf("KSC-008: FAIL sem_init errno=%d\n", errno);
		(void)pthread_mutex_destroy(&g_ksc008_lock);
		return -1;
	}
	status = pthread_mutex_lock(&g_ksc008_lock);
	if (status != 0) {
		printf("KSC-008: FAIL pthread_mutex_lock status=%d\n", status);
		failed = 1;
	} else {
		locked = 1;
	}
	if (!failed && (status = pthread_attr_init(&attr)) == 0) {
		attr_ready = 1;
		status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-008: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-008: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	}
	if (!failed && (status = pthread_create(&worker, &attr,
						      ksc008_trylock_worker, NULL)) == 0) {
		created = 1;
	} else if (!failed) {
		printf("KSC-008: FAIL pthread_create status=%d\n", status);
		failed = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC008_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc008_done, &deadline) != 0) {
			printf("KSC-008: FAIL worker completion errno=%d\n", errno);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-008: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (failed && created) {
		(void)pthread_cancel(worker);
	}
	if (created) {
		status = pthread_join(worker, &result);
		if (status != 0 || (!failed && result != &g_ksc008_exit_token)) {
			printf("KSC-008: FAIL pthread_join status=%d result=%p\n", status,
			       result);
			failed = 1;
		}
	}
	if (!failed && g_ksc008_trylock_status != EBUSY) {
		printf("KSC-008: FAIL trylock status=%d expected=%d\n",
		       g_ksc008_trylock_status, EBUSY);
		failed = 1;
	}
	if (locked && pthread_mutex_unlock(&g_ksc008_lock) != 0) {
		printf("KSC-008: FAIL pthread_mutex_unlock\n");
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-008: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if (sem_destroy(&g_ksc008_done) != 0 ||
	    pthread_mutex_destroy(&g_ksc008_lock) != 0) {
		printf("KSC-008: FAIL cleanup\n");
		failed = 1;
	}
	printf("KSC-008: %s mutex trylock status=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", g_ksc008_trylock_status,
	       (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-009: a condition broadcast must wake every worker that has published
 * itself as waiting. Workers receive all-active-CPU affinity; creator waits
 * are bounded and failure cleanup broadcasts before joining every worker. */
static pthread_addr_t ksc009_broadcast_worker(pthread_addr_t arg)
{
	struct timespec deadline;
	int status;

	(void)arg;
	if (pthread_mutex_lock(&g_ksc009_lock) != 0) {
		return NULL;
	}
	if (sem_post(&g_ksc009_waiting) != 0 ||
	    clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
		(void)pthread_mutex_unlock(&g_ksc009_lock);
		return NULL;
	}
	deadline.tv_sec += KSC009_TIMEOUT_SECONDS;
	while (!g_ksc009_predicate) {
		status = pthread_cond_timedwait(&g_ksc009_condition, &g_ksc009_lock,
						       &deadline);
		if (status != 0) {
			(void)pthread_mutex_unlock(&g_ksc009_lock);
			return NULL;
		}
	}
	g_ksc009_woken++;
	if (pthread_mutex_unlock(&g_ksc009_lock) != 0 ||
	    sem_post(&g_ksc009_done) != 0) {
		return NULL;
	}
	return &g_ksc009_exit_token;
}

static int ksc009_condition_broadcast(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t workers[KSC009_WORKER_COUNT];
	pthread_addr_t result = NULL;
	cpu_set_t mask = 0;
	int attr_ready = 0;
	int created = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-009: START condition broadcast (timeout=%d s)\n",
	       KSC009_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-009: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	g_ksc009_predicate = 0;
	g_ksc009_woken = 0;
	status = pthread_mutex_init(&g_ksc009_lock, NULL);
	if (status != 0) {
		printf("KSC-009: FAIL pthread_mutex_init status=%d\n", status);
		return -1;
	}
	status = pthread_cond_init(&g_ksc009_condition, NULL);
	if (status != 0) {
		printf("KSC-009: FAIL pthread_cond_init status=%d\n", status);
		(void)pthread_mutex_destroy(&g_ksc009_lock);
		return -1;
	}
	if (sem_init(&g_ksc009_waiting, 0, 0) != 0) {
		printf("KSC-009: FAIL waiting sem_init errno=%d\n", errno);
		(void)pthread_cond_destroy(&g_ksc009_condition);
		(void)pthread_mutex_destroy(&g_ksc009_lock);
		return -1;
	}
	if (sem_init(&g_ksc009_done, 0, 0) != 0) {
		printf("KSC-009: FAIL done sem_init errno=%d\n", errno);
		(void)sem_destroy(&g_ksc009_waiting);
		(void)pthread_cond_destroy(&g_ksc009_condition);
		(void)pthread_mutex_destroy(&g_ksc009_lock);
		return -1;
	}
	status = pthread_attr_init(&attr);
	if (status != 0) {
		printf("KSC-009: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	} else {
		attr_ready = 1;
		status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-009: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	}
	for (i = 0; !failed && i < KSC009_WORKER_COUNT; i++) {
		status = pthread_create(&workers[i], &attr, ksc009_broadcast_worker, NULL);
		if (status != 0) {
			printf("KSC-009: FAIL pthread_create[%d] status=%d\n", i, status);
			failed = 1;
		} else {
			created++;
		}
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC009_TIMEOUT_SECONDS;
		for (i = 0; i < KSC009_WORKER_COUNT; i++) {
			if (sem_timedwait(&g_ksc009_waiting, &deadline) != 0) {
				printf("KSC-009: FAIL worker waiting errno=%d\n", errno);
				failed = 1;
				break;
			}
		}
	} else if (!failed) {
		printf("KSC-009: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (pthread_mutex_lock(&g_ksc009_lock) == 0) {
		g_ksc009_predicate = 1;
		if (pthread_cond_broadcast(&g_ksc009_condition) != 0) {
			printf("KSC-009: FAIL pthread_cond_broadcast\n");
			failed = 1;
		}
		if (pthread_mutex_unlock(&g_ksc009_lock) != 0) {
			printf("KSC-009: FAIL pthread_mutex_unlock\n");
			failed = 1;
		}
	} else {
		printf("KSC-009: FAIL pthread_mutex_lock\n");
		failed = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC009_TIMEOUT_SECONDS;
		for (i = 0; i < created; i++) {
			if (sem_timedwait(&g_ksc009_done, &deadline) != 0) {
				printf("KSC-009: FAIL worker completion errno=%d\n", errno);
				failed = 1;
				break;
			}
		}
	} else if (!failed) {
		printf("KSC-009: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	for (i = 0; i < created; i++) {
		status = pthread_join(workers[i], &result);
		if (status != 0 || result != &g_ksc009_exit_token) {
			printf("KSC-009: FAIL pthread_join[%d] status=%d result=%p\n", i,
			       status, result);
			failed = 1;
		}
	}
	if (created != KSC009_WORKER_COUNT || g_ksc009_woken != KSC009_WORKER_COUNT) {
		printf("KSC-009: FAIL woken=%d expected=%d\n", g_ksc009_woken,
		       KSC009_WORKER_COUNT);
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-009: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if (sem_destroy(&g_ksc009_done) != 0 || sem_destroy(&g_ksc009_waiting) != 0 ||
	    pthread_cond_destroy(&g_ksc009_condition) != 0 ||
	    pthread_mutex_destroy(&g_ksc009_lock) != 0) {
		printf("KSC-009: FAIL cleanup\n");
		failed = 1;
	}
	printf("KSC-009: %s condition broadcast woken=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", g_ksc009_woken, (unsigned long)mask);
	return failed ? -1 : 0;
}

/****************************************************************************
 * hello_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int hello_main(int argc, char *argv[])
#endif
{
	int failed = 0;

	(void)argc;
	(void)argv;
	printf("KSC: qemu-virt kernel scenario harness start\n");

	if (ksc001_task_lifecycle() != 0) {
		failed++;
	}
	if (ksc002_mutex_contention() != 0) {
		failed++;
	}
	if (ksc003_condition_wakeup() != 0) {
		failed++;
	}
	if (ksc004_thread_specific_data() != 0) {
		failed++;
	}
	if (ksc005_once_concurrency() != 0) {
		failed++;
	}
	if (ksc006_rwlock_reader_sharing() != 0) {
		failed++;
	}
	if (ksc007_barrier_serial_thread() != 0) {
		failed++;
	}
	if (ksc008_mutex_trylock_busy() != 0) {
		failed++;
	}
	if (ksc009_condition_broadcast() != 0) {
		failed++;
	}

	printf("KSC: harness %s (%d failed)\n", failed ? "FAIL" : "PASS", failed);
	return failed ? 1 : 0;
}
