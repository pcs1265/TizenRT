/****************************************************************************
 *
 * Copyright 2024 Samsung Electronics All Rights Reserved.
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

#include <TR_Utils/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <system/system.h>
#include <system/system_silent_reboot.h>

#include <tinyara/silent_reboot.h>

static void silent_reboot_show_usage(void)
{
	printf("\nUsage: silent_reboot lock | unlock | delay <delay_time(s)> | expire <timeout(s)> | info\n");
	printf("lock                  Lock to prevent silent reboot\n");
	printf("unlock                Unlock to allow silent reboot\n");
	printf("delay <delay_time(s)> Set time to prevent silent reboot for a few seconds (unit:s)\n");
	printf("expire <timeout(s)>   Expire timer to perform silent reboot for a few seconds (unit:s)\n");
	printf("                      It performs silent reboot forcedly if there is no lock regardless of the time after timeout.\n");
	printf("info                  Show silent reboot status\n");
	printf("\n");
}

static void silent_reboot_show_status(void)
{
	int days;
	int hours;
	int minutes;
	int seconds;
	system_result ret;
	silent_reboot_status_t status;

	ret = system_silent_reboot_get_status(&status);
	if (ret < 0) {
		printf("Failed to get silent reboot status\n");
	}

	printf("======= Silent Reboot Status =======\n");
	printf("Lock count : %d\n", status.lock_count);
	printf("Delayed time: %d s\n", status.reboot_delay_left);
	if (status.reboot_timezone_left > 0) {
		seconds = status.reboot_timezone_left;
		days = seconds / (24 * 60 * 60);
		seconds %= (24 * 60 * 60);

		hours = seconds / (60 * 60);
		seconds %= (60 * 60);

		minutes = seconds / 60;
		seconds %= 60;

		printf("Remaining time for periodic reboot: %d days, %dh:%dm:%ds\n", days, hours, minutes, seconds);
	} else {
		printf("Now periodic reboot time zone!!\n");
	}
}

int utils_silent_reboot(int argc, char **args)
{
	system_result ret;

	if (argc >= 2) {
		if (!strncmp(args[1], "--help", strlen("--help") + 1)) {
			goto show_usage;
		}

		if (!strncmp(args[1], "info", strlen("info") + 1)) {
			/* Show silent reboot status */
			silent_reboot_show_status();
			return OK;
		}

		if (!strncmp(args[1], "lock", strlen("lock") + 1)) {
			/* Lock silent reboot */
			ret = system_silent_reboot_lock();
			if (ret == SYSTEM_SUCCESS) {
				printf("Silent reboot Locked\n");
				return OK;
			} else {
				printf("Silent reboot Lock failed\n");
				return ERROR;
			}
		}

		if (!strncmp(args[1], "unlock", strlen("lock") + 1)) {
			/* Unlock silent reboot */
			ret = system_silent_reboot_unlock();
			if (ret == SYSTEM_SUCCESS) {
				printf("Silent reboot Unlocked\n");
				return OK;
			} else {
				printf("Silent reboot Unlock failed\n");
				return ERROR;
			}
		}
 
		if (!strncmp(args[1], "delay", strlen("delay") + 1)) {
			if (argc >= 3) {
				int timeout = atoi(args[2]);
				/* Delay silent reboot */
				ret = system_silent_reboot_delay(timeout);
				if (ret == SYSTEM_SUCCESS) {
					printf("Silent reboot Delayed for %d seconds\n", timeout);
					return OK;
				} else {
					printf("Silent reboot Delay failed. timeout %d\n", timeout);
					return ERROR;
				}
			}
		}

		if (!strncmp(args[1], "expire", strlen("expire") + 1)) {
			if (argc >= 3) {
				int timeout = atoi(args[2]);
				/* Force silent reboot regardless of the time if there is no lock after timeout. */
				ret = system_silent_reboot_force_reboot(timeout);
				if (ret == SYSTEM_SUCCESS) {
					printf("Silent reboot will be performed after %ds if there is no lock\n", timeout);
					return OK;
				} else {
					printf("Failed to expire time to reboot %d\n", ret);
					return ERROR;
				}
			}
			
		}
	}

show_usage:
	silent_reboot_show_usage();

	return ERROR;
}
