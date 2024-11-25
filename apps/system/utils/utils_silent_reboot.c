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
#include <system/system.h>
#include <system/system_silent_reboot.h>

#include <tinyara/silent_reboot.h>

int utils_silent_reboot(int argc, char **args)
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

	return OK;
}
