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
#define KSC010_TIMEOUT_SECONDS 2
#define KSC011_TIMEOUT_SECONDS 2
#define KSC012_TIMEOUT_SECONDS 2
#define KSC013_TIMEOUT_SECONDS 2
#define KSC014_TIMEOUT_SECONDS 2
#define KSC015_TIMEOUT_SECONDS 2
#define KSC016_TIMEOUT_SECONDS 2
#define KSC017_TIMEOUT_SECONDS 2
#define KSC018_TIMEOUT_SECONDS 2
#define KSC019_TIMEOUT_SECONDS 2
#define KSC020_TIMEOUT_SECONDS 2
#define KSC021_TIMEOUT_SECONDS 2
#define KSC022_TIMEOUT_SECONDS 2
#define KSC023_TIMEOUT_SECONDS 2
#define KSC024_TIMEOUT_SECONDS 2
#define KSC025_TIMEOUT_SECONDS 2
#define KSC026_TIMEOUT_SECONDS 2

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
static pthread_rwlock_t g_ksc010_lock;
static sem_t g_ksc010_done;
static int g_ksc010_trywrite_status;
static int g_ksc010_exit_token;
static pthread_rwlock_t g_ksc011_lock;
static sem_t g_ksc011_done;
static int g_ksc011_tryread_status;
static int g_ksc011_exit_token;
static sem_t g_ksc012_empty;
static pthread_mutex_t g_ksc013_lock;
static sem_t g_ksc013_done;
static int g_ksc013_worker_status;
static int g_ksc013_exit_token;
static pthread_mutex_t g_ksc014_lock;
static pthread_cond_t g_ksc014_condition;
static sem_t g_ksc015_done;
static int g_ksc015_exit_token;
static sem_t g_ksc016_probe;
static sem_t g_ksc016_done;
static int g_ksc016_trywait_status;
static int g_ksc016_trywait_errno;
static int g_ksc016_exit_token;
static sem_t g_ksc017_start;
static sem_t g_ksc017_done;
static int g_ksc017_exit_token;
static sem_t g_ksc018_value;
static sem_t g_ksc019_value;
static pthread_mutex_t g_ksc020_lock;
static sem_t g_ksc020_attempting;
static sem_t g_ksc020_done;
static int g_ksc020_worker_status;
static int g_ksc020_exit_token;
static sem_t g_ksc021_done;
static int g_ksc021_equal_status;
static int g_ksc021_exit_token;
static sem_t g_ksc022_waiting;
static sem_t g_ksc022_release;
static sem_t g_ksc022_done;
static int g_ksc022_worker_status;
static int g_ksc022_exit_token;
static pthread_mutex_t g_ksc023_lock;
static pthread_cond_t g_ksc023_condition;
static pthread_mutex_t g_ksc024_lock;
static pthread_rwlock_t g_ksc025_lock;
static sem_t g_ksc025_ready;
static sem_t g_ksc025_release;
static sem_t g_ksc025_done;
static int g_ksc025_worker_status;
static int g_ksc025_exit_token;
static pthread_key_t g_ksc026_key;
static sem_t g_ksc026_destructed;
static int g_ksc026_destructor_count;
static int g_ksc026_value;
static int g_ksc026_exit_token;

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

/* KSC-010: a writer must observe EBUSY while its creator holds a read lock.
 * The worker uses all-active-CPU affinity and reports its nonblocking result
 * through a bounded semaphore wait before the reader is released. */
static pthread_addr_t ksc010_trywrite_worker(pthread_addr_t arg)
{
	(void)arg;
	g_ksc010_trywrite_status = pthread_rwlock_trywrlock(&g_ksc010_lock);
	if (g_ksc010_trywrite_status == 0) {
		(void)pthread_rwlock_unlock(&g_ksc010_lock);
	}
	if (sem_post(&g_ksc010_done) != 0) {
		return NULL;
	}
	return &g_ksc010_exit_token;
}

static int ksc010_rwlock_trywrite_busy(void)
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

	printf("KSC-010: START rwlock writer exclusion (timeout=%d s)\n",
	       KSC010_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-010: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	g_ksc010_trywrite_status = -1;
	status = pthread_rwlock_init(&g_ksc010_lock, NULL);
	if (status != 0) {
		printf("KSC-010: FAIL pthread_rwlock_init status=%d\n", status);
		return -1;
	}
	if (sem_init(&g_ksc010_done, 0, 0) != 0) {
		printf("KSC-010: FAIL sem_init errno=%d\n", errno);
		(void)pthread_rwlock_destroy(&g_ksc010_lock);
		return -1;
	}
	status = pthread_rwlock_rdlock(&g_ksc010_lock);
	if (status != 0) {
		printf("KSC-010: FAIL pthread_rwlock_rdlock status=%d\n", status);
		failed = 1;
	} else {
		locked = 1;
	}
	if (!failed && (status = pthread_attr_init(&attr)) == 0) {
		attr_ready = 1;
		status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-010: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-010: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	}
	if (!failed && (status = pthread_create(&worker, &attr,
						      ksc010_trywrite_worker, NULL)) == 0) {
		created = 1;
	} else if (!failed) {
		printf("KSC-010: FAIL pthread_create status=%d\n", status);
		failed = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC010_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc010_done, &deadline) != 0) {
			printf("KSC-010: FAIL worker completion errno=%d\n", errno);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-010: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (failed && created) {
		(void)pthread_cancel(worker);
	}
	if (created) {
		status = pthread_join(worker, &result);
		if (status != 0 || (!failed && result != &g_ksc010_exit_token)) {
			printf("KSC-010: FAIL pthread_join status=%d result=%p\n", status,
			       result);
			failed = 1;
		}
	}
	if (!failed && g_ksc010_trywrite_status != EBUSY) {
		printf("KSC-010: FAIL trywrlock status=%d expected=%d\n",
		       g_ksc010_trywrite_status, EBUSY);
		failed = 1;
	}
	if (locked && pthread_rwlock_unlock(&g_ksc010_lock) != 0) {
		printf("KSC-010: FAIL pthread_rwlock_unlock\n");
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-010: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if (sem_destroy(&g_ksc010_done) != 0 ||
	    pthread_rwlock_destroy(&g_ksc010_lock) != 0) {
		printf("KSC-010: FAIL cleanup\n");
		failed = 1;
	}
	printf("KSC-010: %s rwlock trywrite status=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", g_ksc010_trywrite_status,
	       (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-011: a reader must observe EBUSY while its creator holds a write lock.
 * This reverses KSC-010's reader-held/writer-rejected direction. The worker
 * uses all-active-CPU affinity and reports its result through a bounded wait. */
static pthread_addr_t ksc011_tryread_worker(pthread_addr_t arg)
{
	(void)arg;
	g_ksc011_tryread_status = pthread_rwlock_tryrdlock(&g_ksc011_lock);
	if (g_ksc011_tryread_status == 0) {
		(void)pthread_rwlock_unlock(&g_ksc011_lock);
	}
	if (sem_post(&g_ksc011_done) != 0) {
		return NULL;
	}
	return &g_ksc011_exit_token;
}

static int ksc011_rwlock_tryread_busy(void)
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

	printf("KSC-011: START rwlock reader exclusion (timeout=%d s)\n",
	       KSC011_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-011: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	g_ksc011_tryread_status = -1;
	status = pthread_rwlock_init(&g_ksc011_lock, NULL);
	if (status != 0) {
		printf("KSC-011: FAIL pthread_rwlock_init status=%d\n", status);
		return -1;
	}
	if (sem_init(&g_ksc011_done, 0, 0) != 0) {
		printf("KSC-011: FAIL sem_init errno=%d\n", errno);
		(void)pthread_rwlock_destroy(&g_ksc011_lock);
		return -1;
	}
	status = pthread_rwlock_wrlock(&g_ksc011_lock);
	if (status != 0) {
		printf("KSC-011: FAIL pthread_rwlock_wrlock status=%d\n", status);
		failed = 1;
	} else {
		locked = 1;
	}
	if (!failed && (status = pthread_attr_init(&attr)) == 0) {
		attr_ready = 1;
		status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-011: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-011: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	}
	if (!failed && (status = pthread_create(&worker, &attr,
						      ksc011_tryread_worker, NULL)) == 0) {
		created = 1;
	} else if (!failed) {
		printf("KSC-011: FAIL pthread_create status=%d\n", status);
		failed = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC011_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc011_done, &deadline) != 0) {
			printf("KSC-011: FAIL worker completion errno=%d\n", errno);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-011: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (failed && created) {
		(void)pthread_cancel(worker);
	}
	if (created) {
		status = pthread_join(worker, &result);
		if (status != 0 || (!failed && result != &g_ksc011_exit_token)) {
			printf("KSC-011: FAIL pthread_join status=%d result=%p\n", status,
			       result);
			failed = 1;
		}
	}
	if (!failed && g_ksc011_tryread_status != EBUSY) {
		printf("KSC-011: FAIL tryrdlock status=%d expected=%d\n",
		       g_ksc011_tryread_status, EBUSY);
		failed = 1;
	}
	if (locked && pthread_rwlock_unlock(&g_ksc011_lock) != 0) {
		printf("KSC-011: FAIL pthread_rwlock_unlock\n");
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-011: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if (sem_destroy(&g_ksc011_done) != 0 ||
	    pthread_rwlock_destroy(&g_ksc011_lock) != 0) {
		printf("KSC-011: FAIL cleanup\n");
		failed = 1;
	}
	printf("KSC-011: %s rwlock tryread status=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", g_ksc011_tryread_status,
	       (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-012: an empty semaphore's finite wait must expire with ETIMEDOUT.
 * This is an isolated kernel timeout path; the semaphore is destroyed after
 * the result is observed. */
static int ksc012_semaphore_timeout(void)
{
	struct timespec deadline;
	int failed = 0;

	printf("KSC-012: START semaphore timeout (timeout=%d s)\n",
	       KSC012_TIMEOUT_SECONDS);
	if (sem_init(&g_ksc012_empty, 0, 0) != 0) {
		printf("KSC-012: FAIL sem_init errno=%d\n", errno);
		return -1;
	}
	if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
		printf("KSC-012: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	} else {
		deadline.tv_sec += KSC012_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc012_empty, &deadline) == 0) {
			printf("KSC-012: FAIL sem_timedwait unexpectedly acquired\n");
			failed = 1;
		} else if (errno != ETIMEDOUT) {
			printf("KSC-012: FAIL sem_timedwait errno=%d expected=%d\n",
			       errno, ETIMEDOUT);
			failed = 1;
		}
	}
	if (sem_destroy(&g_ksc012_empty) != 0) {
		printf("KSC-012: FAIL sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	printf("KSC-012: %s semaphore timeout errno=%d\n",
	       failed ? "FAIL" : "PASS", ETIMEDOUT);
	return failed ? -1 : 0;
}

/* KSC-013: a recursive mutex owner must be able to acquire and release the
 * same mutex twice. The worker has all-active-CPU affinity and reports its
 * bounded completion before the mutex and its attributes are destroyed. */
static pthread_addr_t ksc013_recursive_worker(pthread_addr_t arg)
{
	int locks = 0;
	int status;

	(void)arg;
	status = pthread_mutex_lock(&g_ksc013_lock);
	if (status == 0) {
		locks++;
		status = pthread_mutex_lock(&g_ksc013_lock);
		if (status == 0) {
			locks++;
		}
	}
	while (locks > 0) {
		int unlock_status = pthread_mutex_unlock(&g_ksc013_lock);
		locks--;
		if (status == 0 && unlock_status != 0) {
			status = unlock_status;
		}
	}
	g_ksc013_worker_status = status;
	if (sem_post(&g_ksc013_done) != 0) {
		return NULL;
	}
	return status == 0 ? &g_ksc013_exit_token : NULL;
}

static int ksc013_recursive_mutex(void)
{
	struct timespec deadline;
	pthread_mutexattr_t mutex_attr;
	pthread_attr_t thread_attr;
	pthread_t worker;
	pthread_addr_t result = NULL;
	cpu_set_t mask = 0;
	int mutex_attr_ready = 0;
	int thread_attr_ready = 0;
	int mutex_ready = 0;
	int done_ready = 0;
	int created = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-013: START recursive mutex ownership (timeout=%d s)\n",
	       KSC013_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-013: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	g_ksc013_worker_status = -1;
	status = pthread_mutexattr_init(&mutex_attr);
	if (status != 0) {
		printf("KSC-013: FAIL pthread_mutexattr_init status=%d\n", status);
		return -1;
	}
	mutex_attr_ready = 1;
	status = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	if (status != 0) {
		printf("KSC-013: FAIL pthread_mutexattr_settype status=%d\n", status);
		failed = 1;
	}
	if (!failed && (status = pthread_mutex_init(&g_ksc013_lock, &mutex_attr)) == 0) {
		mutex_ready = 1;
	} else if (!failed) {
		printf("KSC-013: FAIL pthread_mutex_init status=%d\n", status);
		failed = 1;
	}
	if (!failed && sem_init(&g_ksc013_done, 0, 0) == 0) {
		done_ready = 1;
	} else if (!failed) {
		printf("KSC-013: FAIL sem_init errno=%d\n", errno);
		failed = 1;
	}
	if (!failed && (status = pthread_attr_init(&thread_attr)) == 0) {
		thread_attr_ready = 1;
		status = pthread_attr_setaffinity_np(&thread_attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-013: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-013: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	}
	if (!failed && (status = pthread_create(&worker, &thread_attr,
						      ksc013_recursive_worker, NULL)) == 0) {
		created = 1;
	} else if (!failed) {
		printf("KSC-013: FAIL pthread_create status=%d\n", status);
		failed = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC013_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc013_done, &deadline) != 0) {
			printf("KSC-013: FAIL worker completion errno=%d\n", errno);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-013: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (failed && created) {
		(void)pthread_cancel(worker);
	}
	if (created) {
		status = pthread_join(worker, &result);
		if (status != 0 || (!failed && result != &g_ksc013_exit_token)) {
			printf("KSC-013: FAIL pthread_join status=%d result=%p\n", status,
			       result);
			failed = 1;
		}
	}
	if (!failed && g_ksc013_worker_status != 0) {
		printf("KSC-013: FAIL recursive status=%d\n", g_ksc013_worker_status);
		failed = 1;
	}
	if (thread_attr_ready && pthread_attr_destroy(&thread_attr) != 0) {
		printf("KSC-013: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if (done_ready && sem_destroy(&g_ksc013_done) != 0) {
		printf("KSC-013: FAIL sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	if (mutex_ready && pthread_mutex_destroy(&g_ksc013_lock) != 0) {
		printf("KSC-013: FAIL pthread_mutex_destroy\n");
		failed = 1;
	}
	if (mutex_attr_ready && pthread_mutexattr_destroy(&mutex_attr) != 0) {
		printf("KSC-013: FAIL pthread_mutexattr_destroy\n");
		failed = 1;
	}
	printf("KSC-013: %s recursive mutex status=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", g_ksc013_worker_status,
	       (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-014: an unsignaled condition variable's finite wait must expire with
 * ETIMEDOUT.  This exercises the condition wait queue timeout path separately
 * from KSC-003's predicate signal and KSC-009's broadcast wakeup paths. */
static int ksc014_condition_timeout(void)
{
	struct timespec deadline;
	int lock_ready = 0;
	int condition_ready = 0;
	int locked = 0;
	int wait_status = -1;
	int status;
	int failed = 0;

	printf("KSC-014: START condition timeout (timeout=%d s)\n",
	       KSC014_TIMEOUT_SECONDS);
	status = pthread_mutex_init(&g_ksc014_lock, NULL);
	if (status != 0) {
		printf("KSC-014: FAIL pthread_mutex_init status=%d\n", status);
		return -1;
	}
	lock_ready = 1;
	status = pthread_cond_init(&g_ksc014_condition, NULL);
	if (status != 0) {
		printf("KSC-014: FAIL pthread_cond_init status=%d\n", status);
		failed = 1;
	} else {
		condition_ready = 1;
	}
	if (!failed && (status = pthread_mutex_lock(&g_ksc014_lock)) != 0) {
		printf("KSC-014: FAIL pthread_mutex_lock status=%d\n", status);
		failed = 1;
	} else if (!failed) {
		locked = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC014_TIMEOUT_SECONDS;
		wait_status = pthread_cond_timedwait(&g_ksc014_condition,
							    &g_ksc014_lock, &deadline);
		if (wait_status != ETIMEDOUT) {
			printf("KSC-014: FAIL pthread_cond_timedwait status=%d expected=%d\n",
			       wait_status, ETIMEDOUT);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-014: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (locked && pthread_mutex_unlock(&g_ksc014_lock) != 0) {
		printf("KSC-014: FAIL pthread_mutex_unlock\n");
		failed = 1;
	}
	if (condition_ready && pthread_cond_destroy(&g_ksc014_condition) != 0) {
		printf("KSC-014: FAIL pthread_cond_destroy\n");
		failed = 1;
	}
	if (lock_ready && pthread_mutex_destroy(&g_ksc014_lock) != 0) {
		printf("KSC-014: FAIL pthread_mutex_destroy\n");
		failed = 1;
	}
	printf("KSC-014: %s condition timeout status=%d\n",
	       failed ? "FAIL" : "PASS", wait_status);
	return failed ? -1 : 0;
}

/* KSC-015: detached pthread lifecycle completion with a bounded observation. */
static pthread_addr_t ksc015_detached_worker(pthread_addr_t arg)
{
	(void)arg;
	return sem_post(&g_ksc015_done) == 0 ? &g_ksc015_exit_token : NULL;
}

static int ksc015_detached_thread(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t worker;
	cpu_set_t mask = 0;
	int attr_ready = 0;
	int done_ready = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-015: START detached thread completion (timeout=%d s)\n",
	       KSC015_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-015: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	if (sem_init(&g_ksc015_done, 0, 0) != 0) {
		printf("KSC-015: FAIL sem_init errno=%d\n", errno);
		return -1;
	}
	done_ready = 1;
	if ((status = pthread_attr_init(&attr)) != 0) {
		printf("KSC-015: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	} else {
		attr_ready = 1;
	}
	if (!failed && (status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask)) != 0) {
		printf("KSC-015: FAIL affinity status=%d mask=0x%lx\n", status, (unsigned long)mask);
		failed = 1;
	}
	if (!failed && (status = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
		printf("KSC-015: FAIL pthread_attr_setdetachstate status=%d\n", status);
		failed = 1;
	}
	if (!failed && (status = pthread_create(&worker, &attr, ksc015_detached_worker, NULL)) != 0) {
		printf("KSC-015: FAIL pthread_create status=%d\n", status);
		failed = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC015_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc015_done, &deadline) != 0) {
			printf("KSC-015: FAIL worker completion errno=%d\n", errno);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-015: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-015: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if (done_ready && sem_destroy(&g_ksc015_done) != 0) {
		printf("KSC-015: FAIL sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	printf("KSC-015: %s detached completion mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-016: an empty semaphore's nonblocking wait must report EAGAIN, while
 * a subsequently posted token must be acquired. The worker has all-active
 * CPU affinity and its completion is bounded before the semaphore is reused. */
static pthread_addr_t ksc016_trywait_worker(pthread_addr_t arg)
{
	(void)arg;
	g_ksc016_trywait_status = sem_trywait(&g_ksc016_probe);
	g_ksc016_trywait_errno = g_ksc016_trywait_status == 0 ? 0 : errno;
	if (sem_post(&g_ksc016_done) != 0) {
		return NULL;
	}
	return &g_ksc016_exit_token;
}

static int ksc016_semaphore_trywait(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t worker;
	pthread_addr_t result = NULL;
	cpu_set_t mask = 0;
	int probe_ready = 0;
	int done_ready = 0;
	int attr_ready = 0;
	int created = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-016: START semaphore trywait state (timeout=%d s)\n",
	       KSC016_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-016: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	g_ksc016_trywait_status = 0;
	g_ksc016_trywait_errno = 0;
	if (sem_init(&g_ksc016_probe, 0, 0) != 0) {
		printf("KSC-016: FAIL probe sem_init errno=%d\n", errno);
		return -1;
	}
	probe_ready = 1;
	if (sem_init(&g_ksc016_done, 0, 0) != 0) {
		printf("KSC-016: FAIL done sem_init errno=%d\n", errno);
		failed = 1;
	} else {
		done_ready = 1;
	}
	if (!failed && (status = pthread_attr_init(&attr)) != 0) {
		printf("KSC-016: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	} else if (!failed) {
		attr_ready = 1;
		status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-016: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	}
	if (!failed && (status = pthread_create(&worker, &attr,
						      ksc016_trywait_worker, NULL)) != 0) {
		printf("KSC-016: FAIL pthread_create status=%d\n", status);
		failed = 1;
	} else if (!failed) {
		created = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC016_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc016_done, &deadline) != 0) {
			printf("KSC-016: FAIL worker completion errno=%d\n", errno);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-016: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (created) {
		status = pthread_join(worker, &result);
		if (status != 0 || (!failed && result != &g_ksc016_exit_token)) {
			printf("KSC-016: FAIL pthread_join status=%d result=%p\n", status,
			       result);
			failed = 1;
		}
	}
	if (!failed && (g_ksc016_trywait_status != -1 ||
				g_ksc016_trywait_errno != EAGAIN)) {
		printf("KSC-016: FAIL empty trywait status=%d errno=%d expected=%d\n",
		       g_ksc016_trywait_status, g_ksc016_trywait_errno, EAGAIN);
		failed = 1;
	}
	if (!failed && (sem_post(&g_ksc016_probe) != 0 ||
				sem_trywait(&g_ksc016_probe) != 0)) {
		printf("KSC-016: FAIL posted trywait errno=%d\n", errno);
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-016: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if (done_ready && sem_destroy(&g_ksc016_done) != 0) {
		printf("KSC-016: FAIL done sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	if (probe_ready && sem_destroy(&g_ksc016_probe) != 0) {
		printf("KSC-016: FAIL probe sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	printf("KSC-016: %s semaphore trywait empty=%d posted=0 mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", g_ksc016_trywait_errno,
	       (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-017: tryjoin must report EBUSY while an affinity-configured worker is
 * deliberately held at a start gate, then ordinary join retrieves its token
 * after the bounded completion path releases it. */
static pthread_addr_t ksc017_tryjoin_worker(pthread_addr_t arg)
{
	(void)arg;
	if (sem_wait(&g_ksc017_start) != 0 || sem_post(&g_ksc017_done) != 0) {
		return NULL;
	}

	return &g_ksc017_exit_token;
}

static int ksc017_tryjoin_busy(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t worker;
	pthread_addr_t result = NULL;
	cpu_set_t mask = 0;
	int start_ready = 0;
	int done_ready = 0;
	int attr_ready = 0;
	int created = 0;
	int released = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-017: START pthread tryjoin busy (timeout=%d s)\n",
	       KSC017_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-017: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	if (sem_init(&g_ksc017_start, 0, 0) != 0) {
		printf("KSC-017: FAIL start sem_init errno=%d\n", errno);
		return -1;
	}
	start_ready = 1;
	if (sem_init(&g_ksc017_done, 0, 0) != 0) {
		printf("KSC-017: FAIL done sem_init errno=%d\n", errno);
		(void)sem_destroy(&g_ksc017_start);
		return -1;
	}
	done_ready = 1;
	if ((status = pthread_attr_init(&attr)) != 0) {
		printf("KSC-017: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	} else {
		attr_ready = 1;
	}
	if (!failed && (status = pthread_attr_setaffinity_np(&attr, sizeof(mask),
										 &mask)) != 0) {
		printf("KSC-017: FAIL affinity status=%d mask=0x%lx\n", status,
		       (unsigned long)mask);
		failed = 1;
	}
	if (!failed && (status = pthread_create(&worker, &attr,
							      ksc017_tryjoin_worker, NULL)) != 0) {
		printf("KSC-017: FAIL pthread_create status=%d\n", status);
		failed = 1;
	} else if (!failed) {
		created = 1;
		status = pthread_tryjoin_np(worker, &result);
		if (status != EBUSY) {
			printf("KSC-017: FAIL pthread_tryjoin_np status=%d expected=%d\n",
			       status, EBUSY);
			failed = 1;
		}
	}
	if (created) {
		if (sem_post(&g_ksc017_start) != 0) {
			printf("KSC-017: FAIL start sem_post errno=%d\n", errno);
			failed = 1;
		} else {
			released = 1;
		}
	}
	if (created && released && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC017_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc017_done, &deadline) != 0) {
			printf("KSC-017: FAIL worker completion errno=%d\n", errno);
			failed = 1;
		}
	} else if (created && released) {
		printf("KSC-017: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (created && !released) {
		status = pthread_cancel(worker);
		if (status != 0) {
			printf("KSC-017: cleanup pthread_cancel status=%d\n", status);
			failed = 1;
		}
	}
	if (created) {
		status = pthread_join(worker, &result);
		if (status != 0 || (released && result != &g_ksc017_exit_token)) {
			printf("KSC-017: FAIL pthread_join status=%d result=%p\n", status,
			       result);
			failed = 1;
		}
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-017: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if ((done_ready && sem_destroy(&g_ksc017_done) != 0) ||
		(start_ready && sem_destroy(&g_ksc017_start) != 0)) {
		printf("KSC-017: FAIL sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	printf("KSC-017: %s pthread tryjoin busy=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", EBUSY, (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-018: semaphore accounting must report the queued token before and
 * after a bounded timed acquisition.  This isolates sem_getvalue() from the
 * empty and trywait paths exercised by KSC-012 and KSC-016. */
static int ksc018_semaphore_value(void)
{
	struct timespec deadline;
	int before = -1;
	int after = -1;
	int failed = 0;

	printf("KSC-018: START semaphore value accounting (timeout=%d s)\n",
	       KSC018_TIMEOUT_SECONDS);
	if (sem_init(&g_ksc018_value, 0, 0) != 0) {
		printf("KSC-018: FAIL sem_init errno=%d\n", errno);
		return -1;
	}
	if (sem_post(&g_ksc018_value) != 0) {
		printf("KSC-018: FAIL sem_post errno=%d\n", errno);
		failed = 1;
	}
	if (!failed && sem_getvalue(&g_ksc018_value, &before) != 0) {
		printf("KSC-018: FAIL sem_getvalue before errno=%d\n", errno);
		failed = 1;
	}
	if (!failed && before != 1) {
		printf("KSC-018: FAIL value before=%d expected=1\n", before);
		failed = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC018_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc018_value, &deadline) != 0) {
			printf("KSC-018: FAIL sem_timedwait errno=%d\n", errno);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-018: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (sem_getvalue(&g_ksc018_value, &after) != 0) {
		printf("KSC-018: FAIL sem_getvalue after errno=%d\n", errno);
		failed = 1;
	} else if (after != 0) {
		printf("KSC-018: FAIL value after=%d expected=0\n", after);
		failed = 1;
	}
	if (sem_destroy(&g_ksc018_value) != 0) {
		printf("KSC-018: FAIL sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	printf("KSC-018: %s semaphore value before=%d after=%d\n",
	       failed ? "FAIL" : "PASS", before, after);
	return failed ? -1 : 0;
}

/* KSC-019: two queued semaphore tokens must be counted and consumed in
 * sequence.  It extends KSC-018's single-token accounting with bounded waits. */
static int ksc019_semaphore_multiple_tokens(void)
{
	struct timespec deadline;
	int initial = -1;
	int middle = -1;
	int final = -1;
	int failed = 0;

	printf("KSC-019: START semaphore multiple tokens (timeout=%d s)\n",
	       KSC019_TIMEOUT_SECONDS);
	if (sem_init(&g_ksc019_value, 0, 0) != 0) {
		printf("KSC-019: FAIL sem_init errno=%d\n", errno);
		return -1;
	}
	if (sem_post(&g_ksc019_value) != 0 ||
		sem_post(&g_ksc019_value) != 0) {
		printf("KSC-019: FAIL sem_post errno=%d\n", errno);
		failed = 1;
	}
	if (!failed && sem_getvalue(&g_ksc019_value, &initial) != 0) {
		printf("KSC-019: FAIL initial sem_getvalue errno=%d\n", errno);
		failed = 1;
	} else if (!failed && initial != 2) {
		printf("KSC-019: FAIL initial value=%d expected=2\n", initial);
		failed = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC019_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc019_value, &deadline) != 0) {
			printf("KSC-019: FAIL first sem_timedwait errno=%d\n", errno);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-019: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (!failed && sem_getvalue(&g_ksc019_value, &middle) != 0) {
		printf("KSC-019: FAIL middle sem_getvalue errno=%d\n", errno);
		failed = 1;
	} else if (!failed && middle != 1) {
		printf("KSC-019: FAIL middle value=%d expected=1\n", middle);
		failed = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC019_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc019_value, &deadline) != 0) {
			printf("KSC-019: FAIL second sem_timedwait errno=%d\n", errno);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-019: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (sem_getvalue(&g_ksc019_value, &final) != 0) {
		printf("KSC-019: FAIL final sem_getvalue errno=%d\n", errno);
		failed = 1;
	} else if (final != 0) {
		printf("KSC-019: FAIL final value=%d expected=0\n", final);
		failed = 1;
	}
	if (sem_destroy(&g_ksc019_value) != 0) {
		printf("KSC-019: FAIL sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	printf("KSC-019: %s semaphore values initial=%d middle=%d final=%d\n",
	       failed ? "FAIL" : "PASS", initial, middle, final);
	return failed ? -1 : 0;
}

/* KSC-020: a worker publishes its imminent blocking mutex acquisition while
 * the creator holds the mutex. The creator bounds that observation, releases
 * the mutex, then bounds and joins the resulting ownership handoff. */
static pthread_addr_t ksc020_blocking_mutex_worker(pthread_addr_t arg)
{
	int status;

	(void)arg;
	if (sem_post(&g_ksc020_attempting) != 0) {
		return NULL;
	}
	status = pthread_mutex_lock(&g_ksc020_lock);
	g_ksc020_worker_status = status;
	if (status == 0 && pthread_mutex_unlock(&g_ksc020_lock) != 0) {
		g_ksc020_worker_status = -1;
	}
	if (sem_post(&g_ksc020_done) != 0) {
		return NULL;
	}
	return g_ksc020_worker_status == 0 ? &g_ksc020_exit_token : NULL;
}

static int ksc020_mutex_blocking_handoff(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t worker;
	pthread_addr_t result = NULL;
	cpu_set_t mask = 0;
	int attr_ready = 0;
	int attempting_ready = 0;
	int done_ready = 0;
	int created = 0;
	int locked = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-020: START mutex blocking handoff (timeout=%d s)\n",
	       KSC020_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-020: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	g_ksc020_worker_status = -1;
	status = pthread_mutex_init(&g_ksc020_lock, NULL);
	if (status != 0) {
		printf("KSC-020: FAIL pthread_mutex_init status=%d\n", status);
		return -1;
	}
	if (sem_init(&g_ksc020_attempting, 0, 0) != 0 ||
	    sem_init(&g_ksc020_done, 0, 0) != 0) {
		printf("KSC-020: FAIL sem_init errno=%d\n", errno);
		(void)sem_destroy(&g_ksc020_attempting);
		(void)pthread_mutex_destroy(&g_ksc020_lock);
		return -1;
	}
	attempting_ready = 1;
	done_ready = 1;
	status = pthread_mutex_lock(&g_ksc020_lock);
	if (status != 0) {
		printf("KSC-020: FAIL pthread_mutex_lock status=%d\n", status);
		failed = 1;
	} else {
		locked = 1;
	}
	if (!failed && (status = pthread_attr_init(&attr)) == 0) {
		attr_ready = 1;
		status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-020: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-020: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	}
	if (!failed && (status = pthread_create(&worker, &attr,
						      ksc020_blocking_mutex_worker, NULL)) == 0) {
		created = 1;
	} else if (!failed) {
		printf("KSC-020: FAIL pthread_create status=%d\n", status);
		failed = 1;
	}
	if (created && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC020_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc020_attempting, &deadline) != 0) {
			printf("KSC-020: FAIL worker attempt errno=%d\n", errno);
			failed = 1;
		}
	} else if (created) {
		printf("KSC-020: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	/* Always release a created worker before its bounded completion and join. */
	if (locked && pthread_mutex_unlock(&g_ksc020_lock) != 0) {
		printf("KSC-020: FAIL pthread_mutex_unlock\n");
		failed = 1;
	}
	locked = 0;
	if (created && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC020_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc020_done, &deadline) != 0) {
			printf("KSC-020: FAIL worker completion errno=%d\n", errno);
			failed = 1;
		}
	} else if (created) {
		printf("KSC-020: FAIL completion clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (created) {
		status = pthread_join(worker, &result);
		if (status != 0 || (!failed && result != &g_ksc020_exit_token)) {
			printf("KSC-020: FAIL pthread_join status=%d result=%p\n", status,
			       result);
			failed = 1;
		}
	}
	if (!failed && g_ksc020_worker_status != 0) {
		printf("KSC-020: FAIL worker mutex status=%d\n", g_ksc020_worker_status);
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-020: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if ((done_ready && sem_destroy(&g_ksc020_done) != 0) ||
	    (attempting_ready && sem_destroy(&g_ksc020_attempting) != 0) ||
	    pthread_mutex_destroy(&g_ksc020_lock) != 0) {
		printf("KSC-020: FAIL cleanup\n");
		failed = 1;
	}
	printf("KSC-020: %s mutex blocking handoff status=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", g_ksc020_worker_status,
	       (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-021: an affinity-configured worker's pthread_t identity must compare
 * equal to itself. The creator bounds completion, joins the worker, and
 * destroys its attribute and semaphore on every path. */
static pthread_addr_t ksc021_self_identity_worker(pthread_addr_t arg)
{
	pthread_t self;

	(void)arg;
	self = pthread_self();
	g_ksc021_equal_status = pthread_equal(self, self);
	if (!g_ksc021_equal_status || sem_post(&g_ksc021_done) != 0) {
		return NULL;
	}

	return &g_ksc021_exit_token;
}

static int ksc021_pthread_self_identity(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t worker;
	pthread_addr_t result = NULL;
	cpu_set_t mask = 0;
	int attr_ready = 0;
	int done_ready = 0;
	int created = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-021: START pthread self identity (timeout=%d s)\n",
	       KSC021_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-021: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	g_ksc021_equal_status = 0;
	if (sem_init(&g_ksc021_done, 0, 0) != 0) {
		printf("KSC-021: FAIL sem_init errno=%d\n", errno);
		return -1;
	}
	done_ready = 1;
	if ((status = pthread_attr_init(&attr)) != 0) {
		printf("KSC-021: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	} else {
		attr_ready = 1;
	}
	if (!failed && (status = pthread_attr_setaffinity_np(&attr, sizeof(mask),
										 &mask)) != 0) {
		printf("KSC-021: FAIL affinity status=%d mask=0x%lx\n", status,
		       (unsigned long)mask);
		failed = 1;
	}
	if (!failed && (status = pthread_create(&worker, &attr,
						      ksc021_self_identity_worker, NULL)) != 0) {
		printf("KSC-021: FAIL pthread_create status=%d\n", status);
		failed = 1;
	} else if (!failed) {
		created = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC021_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc021_done, &deadline) != 0) {
			printf("KSC-021: FAIL worker completion errno=%d\n", errno);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-021: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (failed && created) {
		(void)pthread_cancel(worker);
	}
	if (created) {
		status = pthread_join(worker, &result);
		if (status != 0 || (!failed && result != &g_ksc021_exit_token)) {
			printf("KSC-021: FAIL pthread_join status=%d result=%p\n", status,
		       result);
			failed = 1;
		}
	}
	if (!failed && !g_ksc021_equal_status) {
		printf("KSC-021: FAIL pthread_equal status=%d\n", g_ksc021_equal_status);
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-021: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if (done_ready && sem_destroy(&g_ksc021_done) != 0) {
		printf("KSC-021: FAIL sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	printf("KSC-021: %s pthread self identity status=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", g_ksc021_equal_status,
	       (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-022: a worker publishes its pending semaphore wait, then must be woken
 * by exactly one creator post. Both creator observations are bounded and a
 * release token is supplied on every created-worker path before joining. */
static pthread_addr_t ksc022_semaphore_wake_worker(pthread_addr_t arg)
{
	(void)arg;
	if (sem_post(&g_ksc022_waiting) != 0) {
		return NULL;
	}
	g_ksc022_worker_status = sem_wait(&g_ksc022_release);
	if (sem_post(&g_ksc022_done) != 0) {
		return NULL;
	}
	return g_ksc022_worker_status == 0 ? &g_ksc022_exit_token : NULL;
}

static int ksc022_semaphore_wake_handoff(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t worker;
	pthread_addr_t result = NULL;
	cpu_set_t mask = 0;
	int waiting_ready = 0;
	int release_ready = 0;
	int done_ready = 0;
	int attr_ready = 0;
	int created = 0;
	int released = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-022: START semaphore wake handoff (timeout=%d s)\n",
	       KSC022_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-022: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	g_ksc022_worker_status = -1;
	if (sem_init(&g_ksc022_waiting, 0, 0) != 0) {
		printf("KSC-022: FAIL waiting sem_init errno=%d\n", errno);
		return -1;
	}
	waiting_ready = 1;
	if (sem_init(&g_ksc022_release, 0, 0) != 0) {
		printf("KSC-022: FAIL release sem_init errno=%d\n", errno);
		failed = 1;
	} else {
		release_ready = 1;
	}
	if (!failed && sem_init(&g_ksc022_done, 0, 0) != 0) {
		printf("KSC-022: FAIL done sem_init errno=%d\n", errno);
		failed = 1;
	} else if (!failed) {
		done_ready = 1;
	}
	if (!failed && (status = pthread_attr_init(&attr)) != 0) {
		printf("KSC-022: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	} else if (!failed) {
		attr_ready = 1;
		status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-022: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	}
	if (!failed && (status = pthread_create(&worker, &attr,
						      ksc022_semaphore_wake_worker, NULL)) != 0) {
		printf("KSC-022: FAIL pthread_create status=%d\n", status);
		failed = 1;
	} else if (!failed) {
		created = 1;
	}
	if (created && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC022_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc022_waiting, &deadline) != 0) {
			printf("KSC-022: FAIL worker waiting errno=%d\n", errno);
			failed = 1;
		}
	} else if (created) {
		printf("KSC-022: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (created && sem_post(&g_ksc022_release) != 0) {
		printf("KSC-022: FAIL release sem_post errno=%d\n", errno);
		failed = 1;
	} else if (created) {
		released = 1;
	}
	if (created && released && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC022_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc022_done, &deadline) != 0) {
			printf("KSC-022: FAIL worker completion errno=%d\n", errno);
			failed = 1;
		}
	} else if (created && released) {
		printf("KSC-022: FAIL completion clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (created && !released) {
		(void)pthread_cancel(worker);
	}
	if (created) {
		status = pthread_join(worker, &result);
		if (status != 0 || (released && result != &g_ksc022_exit_token)) {
			printf("KSC-022: FAIL pthread_join status=%d result=%p\n", status,
		       result);
			failed = 1;
		}
	}
	if (!failed && g_ksc022_worker_status != 0) {
		printf("KSC-022: FAIL worker sem_wait status=%d\n",
		       g_ksc022_worker_status);
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-022: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if ((done_ready && sem_destroy(&g_ksc022_done) != 0) ||
	    (release_ready && sem_destroy(&g_ksc022_release) != 0) ||
	    (waiting_ready && sem_destroy(&g_ksc022_waiting) != 0)) {
		printf("KSC-022: FAIL sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	printf("KSC-022: %s semaphore wake status=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", g_ksc022_worker_status,
	       (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-023: a condition signal with no waiters must not be retained for a
 * later waiter.  The later timed wait bounds the check and verifies that the
 * condition remains unsignaled. */
static int ksc023_condition_signal_no_waiter(void)
{
	struct timespec deadline;
	int lock_ready = 0;
	int condition_ready = 0;
	int lock_held = 0;
	int wait_status = -1;
	int status;
	int failed = 0;

	printf("KSC-023: START condition signal without waiter (timeout=%d s)\n",
	       KSC023_TIMEOUT_SECONDS);
	status = pthread_mutex_init(&g_ksc023_lock, NULL);
	if (status != 0) {
		printf("KSC-023: FAIL pthread_mutex_init status=%d\n", status);
		return -1;
	}
	lock_ready = 1;
	status = pthread_cond_init(&g_ksc023_condition, NULL);
	if (status != 0) {
		printf("KSC-023: FAIL pthread_cond_init status=%d\n", status);
		failed = 1;
	} else {
		condition_ready = 1;
	}
	if (!failed && (status = pthread_cond_signal(&g_ksc023_condition)) != 0) {
		printf("KSC-023: FAIL pthread_cond_signal status=%d\n", status);
		failed = 1;
	}
	if (!failed && (status = pthread_mutex_lock(&g_ksc023_lock)) != 0) {
		printf("KSC-023: FAIL pthread_mutex_lock status=%d\n", status);
		failed = 1;
	} else if (!failed) {
		lock_held = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
		printf("KSC-023: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	} else if (!failed) {
		deadline.tv_sec += KSC023_TIMEOUT_SECONDS;
		wait_status = pthread_cond_timedwait(&g_ksc023_condition,
							     &g_ksc023_lock, &deadline);
		if (wait_status != ETIMEDOUT) {
			printf("KSC-023: FAIL condition status=%d\n", wait_status);
			failed = 1;
		}
	}
	if (lock_held && (status = pthread_mutex_unlock(&g_ksc023_lock)) != 0) {
		printf("KSC-023: FAIL pthread_mutex_unlock status=%d\n", status);
		failed = 1;
	}
	if (condition_ready && (status = pthread_cond_destroy(&g_ksc023_condition)) != 0) {
		printf("KSC-023: FAIL pthread_cond_destroy status=%d\n", status);
		failed = 1;
	}
	if (lock_ready && (status = pthread_mutex_destroy(&g_ksc023_lock)) != 0) {
		printf("KSC-023: FAIL pthread_mutex_destroy status=%d\n", status);
		failed = 1;
	}
	printf("KSC-023: %s condition signal is not retained status=%d\n",
	       failed ? "FAIL" : "PASS", wait_status);
	return failed ? -1 : 0;
}

/* KSC-024: trylock must acquire an unlocked mutex immediately.  This is the
 * success counterpart to KSC-008's contended EBUSY path, with no worker left
 * running and cleanup performed before returning to the harness. */
static int ksc024_mutex_trylock_available(void)
{
	int lock_ready = 0;
	int lock_held = 0;
	int trylock_status = -1;
	int status;
	int failed = 0;

	printf("KSC-024: START mutex trylock available (timeout=%d s)\n",
	       KSC024_TIMEOUT_SECONDS);
	status = pthread_mutex_init(&g_ksc024_lock, NULL);
	if (status != 0) {
		printf("KSC-024: FAIL pthread_mutex_init status=%d\n", status);
		return -1;
	}
	lock_ready = 1;
	trylock_status = pthread_mutex_trylock(&g_ksc024_lock);
	if (trylock_status != 0) {
		printf("KSC-024: FAIL pthread_mutex_trylock status=%d\n", trylock_status);
		failed = 1;
	} else {
		lock_held = 1;
	}
	if (lock_held && (status = pthread_mutex_unlock(&g_ksc024_lock)) != 0) {
		printf("KSC-024: FAIL pthread_mutex_unlock status=%d\n", status);
		failed = 1;
	}
	if (lock_ready && (status = pthread_mutex_destroy(&g_ksc024_lock)) != 0) {
		printf("KSC-024: FAIL pthread_mutex_destroy status=%d\n", status);
		failed = 1;
	}
	printf("KSC-024: %s mutex trylock status=%d\n",
	       failed ? "FAIL" : "PASS", trylock_status);
	return failed ? -1 : 0;
}

/* KSC-025: a timed read-lock acquisition must expire while another worker
 * owns the write lock.  This covers the bounded blocking rwlock path rather
 * than KSC-010/KSC-011's immediate trylock exclusion paths. */
static pthread_addr_t ksc025_timed_read_worker(pthread_addr_t arg)
{
	int locked = 0;
	int status;

	(void)arg;
	status = pthread_rwlock_wrlock(&g_ksc025_lock);
	if (status == 0) {
		locked = 1;
	}
	if (locked && sem_post(&g_ksc025_ready) != 0) {
		status = -1;
	}
	if (locked && sem_wait(&g_ksc025_release) != 0) {
		status = -1;
	}
	if (locked && pthread_rwlock_unlock(&g_ksc025_lock) != 0) {
		status = -1;
	}
	g_ksc025_worker_status = status;
	if (sem_post(&g_ksc025_done) != 0) {
		return NULL;
	}
	return status == 0 ? &g_ksc025_exit_token : NULL;
}

static int ksc025_rwlock_timedread_timeout(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t worker;
	pthread_addr_t result = NULL;
	cpu_set_t mask = 0;
	int lock_ready = 0;
	int ready_ready = 0;
	int release_ready = 0;
	int done_ready = 0;
	int attr_ready = 0;
	int created = 0;
	int released = 0;
	int timed_status = -1;
	int status;
	int i;
	int failed = 0;

	printf("KSC-025: START rwlock timedread timeout (timeout=%d s)\n",
	       KSC025_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-025: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	g_ksc025_worker_status = -1;
	if ((status = pthread_rwlock_init(&g_ksc025_lock, NULL)) != 0) {
		printf("KSC-025: FAIL pthread_rwlock_init status=%d\n", status);
		return -1;
	}
	lock_ready = 1;
	if (sem_init(&g_ksc025_ready, 0, 0) != 0) {
		printf("KSC-025: FAIL sem_init errno=%d\n", errno);
		failed = 1;
	} else {
		ready_ready = 1;
	}
	if (!failed && sem_init(&g_ksc025_release, 0, 0) != 0) {
		printf("KSC-025: FAIL release sem_init errno=%d\n", errno);
		failed = 1;
	} else if (!failed) {
		release_ready = 1;
	}
	if (!failed && sem_init(&g_ksc025_done, 0, 0) != 0) {
		printf("KSC-025: FAIL done sem_init errno=%d\n", errno);
		failed = 1;
	} else if (!failed) {
		done_ready = 1;
	}
	if (!failed && (status = pthread_attr_init(&attr)) != 0) {
		printf("KSC-025: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	} else if (!failed) {
		attr_ready = 1;
		if ((status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask)) != 0) {
			printf("KSC-025: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	}
	if (!failed && (status = pthread_create(&worker, &attr,
							      ksc025_timed_read_worker, NULL)) != 0) {
		printf("KSC-025: FAIL pthread_create status=%d\n", status);
		failed = 1;
	} else if (!failed) {
		created = 1;
	}
	if (created && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC025_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc025_ready, &deadline) != 0) {
			printf("KSC-025: FAIL worker ready errno=%d\n", errno);
			failed = 1;
		}
	} else if (created) {
		printf("KSC-025: FAIL ready clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (!failed && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC025_TIMEOUT_SECONDS;
		timed_status = pthread_rwlock_timedrdlock(&g_ksc025_lock, &deadline);
		if (timed_status != ETIMEDOUT) {
			printf("KSC-025: FAIL pthread_rwlock_timedrdlock status=%d expected=%d\n",
			       timed_status, ETIMEDOUT);
			failed = 1;
		}
	} else if (!failed) {
		printf("KSC-025: FAIL timedread clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (created && sem_post(&g_ksc025_release) != 0) {
		printf("KSC-025: FAIL release sem_post errno=%d\n", errno);
		failed = 1;
	} else if (created) {
		released = 1;
	}
	if (created && released && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC025_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc025_done, &deadline) != 0) {
			printf("KSC-025: FAIL worker completion errno=%d\n", errno);
			failed = 1;
		}
	} else if (created && released) {
		printf("KSC-025: FAIL completion clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (created) {
		status = pthread_join(worker, &result);
		if (status != 0 || (released && result != &g_ksc025_exit_token)) {
			printf("KSC-025: FAIL pthread_join status=%d result=%p\n", status,
			       result);
			failed = 1;
		}
	}
	if (!failed && g_ksc025_worker_status != 0) {
		printf("KSC-025: FAIL worker status=%d\n", g_ksc025_worker_status);
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-025: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if ((done_ready && sem_destroy(&g_ksc025_done) != 0) ||
	    (release_ready && sem_destroy(&g_ksc025_release) != 0) ||
	    (ready_ready && sem_destroy(&g_ksc025_ready) != 0) ||
	    (lock_ready && pthread_rwlock_destroy(&g_ksc025_lock) != 0)) {
		printf("KSC-025: FAIL cleanup errno=%d\n", errno);
		failed = 1;
	}
	printf("KSC-025: %s rwlock timedread status=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", timed_status, (unsigned long)mask);
	return failed ? -1 : 0;
}

/* KSC-026: a non-NULL thread-specific value must invoke its key destructor
 * when an all-active-CPU-affined worker exits.  This covers the exit cleanup
 * path separately from KSC-004's set/get and key deletion path. */
static void ksc026_key_destructor(FAR void *value)
{
	if (value == &g_ksc026_value) {
		g_ksc026_destructor_count++;
		(void)sem_post(&g_ksc026_destructed);
	}
}

static pthread_addr_t ksc026_destructor_worker(pthread_addr_t arg)
{
	int status;

	(void)arg;
	status = pthread_setspecific(g_ksc026_key, &g_ksc026_value);
	return status == 0 ? &g_ksc026_exit_token : NULL;
}

static int ksc026_tls_destructor(void)
{
	struct timespec deadline;
	pthread_attr_t attr;
	pthread_t worker;
	pthread_addr_t result = NULL;
	cpu_set_t mask = 0;
	int key_ready = 0;
	int done_ready = 0;
	int attr_ready = 0;
	int created = 0;
	int status;
	int i;
	int failed = 0;

	printf("KSC-026: START thread-specific destructor (timeout=%d s)\n",
	       KSC026_TIMEOUT_SECONDS);
	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		mask |= ((cpu_set_t)1 << i);
	}
	printf("KSC-026: worker affinity mask=0x%lx cpus=%d\n",
	       (unsigned long)mask, CONFIG_SMP_NCPUS);
	g_ksc026_destructor_count = 0;
	if (sem_init(&g_ksc026_destructed, 0, 0) != 0) {
		printf("KSC-026: FAIL sem_init errno=%d\n", errno);
		return -1;
	}
	done_ready = 1;
	if ((status = pthread_key_create(&g_ksc026_key, ksc026_key_destructor)) != 0) {
		printf("KSC-026: FAIL pthread_key_create status=%d\n", status);
		failed = 1;
	} else {
		key_ready = 1;
	}
	if (!failed && (status = pthread_attr_init(&attr)) != 0) {
		printf("KSC-026: FAIL pthread_attr_init status=%d\n", status);
		failed = 1;
	} else if (!failed) {
		attr_ready = 1;
		status = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
		if (status != 0) {
			printf("KSC-026: FAIL affinity status=%d mask=0x%lx\n", status,
			       (unsigned long)mask);
			failed = 1;
		}
	}
	if (!failed && (status = pthread_create(&worker, &attr,
						      ksc026_destructor_worker, NULL)) != 0) {
		printf("KSC-026: FAIL pthread_create status=%d\n", status);
		failed = 1;
	} else if (!failed) {
		created = 1;
	}
	if (created && clock_gettime(CLOCK_REALTIME, &deadline) == 0) {
		deadline.tv_sec += KSC026_TIMEOUT_SECONDS;
		if (sem_timedwait(&g_ksc026_destructed, &deadline) != 0) {
			printf("KSC-026: FAIL destructor completion errno=%d\n", errno);
			failed = 1;
		}
	} else if (created) {
		printf("KSC-026: FAIL clock_gettime errno=%d\n", errno);
		failed = 1;
	}
	if (created) {
		status = pthread_join(worker, &result);
		if (status != 0 || result != &g_ksc026_exit_token) {
			printf("KSC-026: FAIL pthread_join status=%d result=%p\n", status,
			       result);
			failed = 1;
		}
	}
	if (g_ksc026_destructor_count != 1) {
		printf("KSC-026: FAIL destructor count=%d expected=1\n",
		       g_ksc026_destructor_count);
		failed = 1;
	}
	if (attr_ready && pthread_attr_destroy(&attr) != 0) {
		printf("KSC-026: FAIL pthread_attr_destroy\n");
		failed = 1;
	}
	if (key_ready && (status = pthread_key_delete(g_ksc026_key)) != 0) {
		printf("KSC-026: FAIL pthread_key_delete status=%d\n", status);
		failed = 1;
	}
	if (done_ready && sem_destroy(&g_ksc026_destructed) != 0) {
		printf("KSC-026: FAIL sem_destroy errno=%d\n", errno);
		failed = 1;
	}
	printf("KSC-026: %s thread-specific destructor count=%d mask=0x%lx\n",
	       failed ? "FAIL" : "PASS", g_ksc026_destructor_count,
	       (unsigned long)mask);
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
	if (ksc010_rwlock_trywrite_busy() != 0) {
		failed++;
	}
	if (ksc011_rwlock_tryread_busy() != 0) {
		failed++;
	}
	if (ksc012_semaphore_timeout() != 0) {
		failed++;
	}
	if (ksc013_recursive_mutex() != 0) {
		failed++;
	}
	if (ksc014_condition_timeout() != 0) {
		failed++;
	}
	if (ksc015_detached_thread() != 0) {
		failed++;
	}
	if (ksc016_semaphore_trywait() != 0) {
		failed++;
	}
	if (ksc017_tryjoin_busy() != 0) {
		failed++;
	}
	if (ksc018_semaphore_value() != 0) {
		failed++;
	}
	if (ksc019_semaphore_multiple_tokens() != 0) {
		failed++;
	}
	if (ksc020_mutex_blocking_handoff() != 0) {
		failed++;
	}
	if (ksc021_pthread_self_identity() != 0) {
		failed++;
	}
	if (ksc022_semaphore_wake_handoff() != 0) {
		failed++;
	}
	if (ksc023_condition_signal_no_waiter() != 0) {
		failed++;
	}
	if (ksc024_mutex_trylock_available() != 0) {
		failed++;
	}
	if (ksc025_rwlock_timedread_timeout() != 0) {
		failed++;
	}
	if (ksc026_tls_destructor() != 0) {
		failed++;
	}

	printf("KSC: harness %s (%d failed)\n", failed ? "FAIL" : "PASS", failed);
	return failed ? 1 : 0;
}
