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

#include <tinyara/config.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <tinyara/silent_reboot.h>

static int silent_reboot_request(int cmd, unsigned long arg)
{
	int fd;
	int ret;

	fd = open(SILENT_REBOOT_DRVPATH, O_RDWR);
	if (fd < 0) {
		printf("Failed to open %s\n", SILENT_REBOOT_DRVPATH);
		return ERROR;
	}

	ret = ioctl(fd, cmd, arg);
	close(fd);

	return ret;
}

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

static int silent_reboot_show_status(void)
{
	int days;
	int hours;
	int minutes;
	int seconds;
	int ret;
	silent_reboot_status_t status;

	ret = silent_reboot_request(SILENTRBIOC_GETSTATUS, (unsigned long)(uintptr_t)&status);
	if (ret < 0) {
		printf("Failed to get silent reboot status\n");
		return ERROR;
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

	return OK;
}

int utils_silent_reboot(int argc, char **args)
{
	int ret;

	if (argc >= 2) {
		if (!strncmp(args[1], "--help", strlen("--help") + 1)) {
			goto show_usage;
		}

		if (!strncmp(args[1], "info", strlen("info") + 1)) {
			/* Show silent reboot status */
			return silent_reboot_show_status();
		}

		if (!strncmp(args[1], "lock", strlen("lock") + 1)) {
			/* Lock silent reboot */
			ret = silent_reboot_request(SILENTRBIOC_LOCK, 0);
			if (ret == OK) {
				printf("Silent reboot Locked\n");
				return OK;
			} else {
				printf("Silent reboot Lock failed\n");
				return ERROR;
			}
		}

		if (!strncmp(args[1], "unlock", strlen("lock") + 1)) {
			/* Unlock silent reboot */
			ret = silent_reboot_request(SILENTRBIOC_UNLOCK, 0);
			if (ret == OK) {
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
				ret = silent_reboot_request(SILENTRBIOC_DELAY, (unsigned long)timeout);
				if (ret == OK) {
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
				ret = silent_reboot_request(SILENTRBIOC_FORCE_REBOOT, (unsigned long)timeout);
				if (ret == OK) {
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
