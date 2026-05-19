/****************************************************************************
 *
 * Copyright 2025 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * arch/arm/src/qemu-virt/qemu_smp.c
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

#include <stdint.h>
#include <assert.h>
#include <debug.h>
#include <errno.h>

#include <tinyara/arch.h>
#include <tinyara/sched.h>

#include "smp.h"
#include "psci.h"

#ifdef CONFIG_SMP

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* CPU entry point lookup table - maps CPU index to its startup entry point.
 * This is shared with qemu_boot.c's smp_init() to avoid duplication.
 */
unsigned long cpu_start[CONFIG_SMP_NCPUS] = {
  0,
#if CONFIG_SMP_NCPUS > 1
  (unsigned long)__cpu1_start,
#endif
#if CONFIG_SMP_NCPUS > 2
  (unsigned long)__cpu2_start,
#endif
#if CONFIG_SMP_NCPUS > 3
  (unsigned long)__cpu3_start,
#endif
};

/* PSCI affinity state values */
typedef enum {
	AFF_STATE_ON = 0,
	AFF_STATE_OFF = 1,
	AFF_STATE_ON_PENDING = 2
} aff_info_state_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_CPU_HOTPLUG
/****************************************************************************
 * Name: cpu_core_powerdown
 *
 * Description:
 *   Powers down the specified CPU core after ensuring it's in a safe state.
 *
 * Input Parameters:
 *   cpu - The CPU ID to power down
 *
 * Returned Value:
 *   OK (0) on success, negated errno on failure
 *
 ****************************************************************************/

static int cpu_core_powerdown(int cpu)
{
	int count = 10;
	int state;

	/* Currently only supports CPU1 */
	if (cpu != 1) {
		smplldbg("%s only supports CPU1, got CPU%d\n", __func__, cpu);
		return -ENOTSUP;
	}

	/* Wait for PSCI to report that CPU1 is now safe to power off */
	do {
		state = psci_affinity_info(cpu, 0);
		if (state == AFF_STATE_OFF) {
			return OK;
		}
		udelay(50);
	} while (count--);

	smplldbg("Core powerdown timeout for CPU%d, affinfo: %d\n", cpu, state);
	return -ETIMEDOUT;
}
#endif /* CONFIG_CPU_HOTPLUG */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_cpu_up
 *
 * Description:
 *   Power on and boot a CPU core that was previously powered down. This function
 *   performs the complete power-on sequence by cold-booting the secondary
 *   core via PSCI to the specified entry point.
 *
 * Input Parameters:
 *   cpu - The index of the CPU to power on
 *
 * Returned Value:
 *   Zero (OK) on success, negated errno on failure
 *
 * Assumptions:
 *   - Called from CPU0
 *   - Target CPU is currently powered off
 *   - System is ready to bring additional CPU online
 *   - PSCI has been initialized
 *
 ****************************************************************************/

int up_cpu_up(int cpu)
{
	int ret;

	if (cpu <= 0 || cpu >= CONFIG_SMP_NCPUS) {
		smplldbg("cpu number out of range\n");
		return -EINVAL;
	}

	smpllvdbg("Starting secondary core CPU%d\n", cpu);

	/* Cold-boot the secondary core via PSCI using the correct entry point */
	ret = psci_cpu_on(cpu, cpu_start[cpu]);
	if (ret < 0) {
		smplldbg("Failed to boot CPU%d: %d\n", cpu, ret);
		return ret;
	}

	smpllvdbg("Secondary core CPU%d started successfully\n", cpu);
	return OK;
}

#ifdef CONFIG_CPU_HOTPLUG

/****************************************************************************
 * Name: up_cpu_die
 *
 * Description:
 *   Shut down the current CPU core using PSCI. This function is called by a
 *   CPU that needs to power itself down as part of the hot-plug sequence.
 *   It invokes the PSCI CPU_OFF service which puts the calling CPU into
 *   a low-power state and prevents it from being scheduled for execution.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Does not return to the caller.
 *
 * Assumptions:
 *   - Called from the target CPU that needs to shut down
 *   - CPU has already completed all necessary cleanup and state saving
 *
 ****************************************************************************/

void up_cpu_die(void)
{
	/* Shut down the cpu using PSCI CPU_OFF */
	(void)psci_cpu_off();
	
	/* Should not return - wait in infinite loop as fallback */
	for (;;) {
		asm("WFI");
	}
}

/****************************************************************************
 * Name: up_cpu_down
 *
 * Description:
 *   Power down a CPU core by coordinating with PSCI. This function
 *   waits for the target CPU to enter OFF state via PSCI affinity info.
 *
 * Input Parameters:
 *   cpu - The index of the CPU to power down
 *
 * Returned Value:
 *   Zero (OK) on success, negated errno on failure
 *
 * Assumptions:
 *   - Called from CPU0
 *   - Target CPU has been hot-plugged out
 *   - All tasks have been migrated away from target CPU
 *
 ****************************************************************************/

int up_cpu_down(int cpu)
{
	int ret;

	if (cpu <= 0 || cpu >= CONFIG_SMP_NCPUS) {
		smplldbg("cpu number out of range\n");
		return -EINVAL;
	}

	smpllvdbg("Starting core powerdown for CPU%d\n", cpu);

	ret = cpu_core_powerdown(cpu);
	if (ret < 0) {
		smplldbg("Failed to powerdown CPU%d, errno: %d\n", cpu, ret);
		return ret;
	}

	smpllvdbg("Secondary core CPU%d powerdown complete\n", cpu);
	return OK;
}

#endif /* CONFIG_CPU_HOTPLUG */

#endif /* CONFIG_SMP */
