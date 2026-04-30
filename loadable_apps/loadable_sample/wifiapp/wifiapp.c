/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
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
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#ifdef CONFIG_BINARY_MANAGER
#include <binary_manager/binary_manager.h>
#endif

#include "wifiapp_internal.h"
/****************************************************************************
 * Public Functions
 ****************************************************************************/


/*
 * Test Secure Storage APIs - Data A/B Toggle
 */
#include <tinyara/seclink.h>
#include <tinyara/security_hal.h>

#define TEST_SS_SLOT_INDEX      5
#define TEST_SS_MAX_DATA_SIZE   8192
#define TEST_SS_DATA_SIZE       2048

/* Test data A and B */
static const unsigned char DATA_A[2048] = { [ 0 ... 2047 ] = 0x55};
static const unsigned char DATA_B[2048] = { [ 0 ... 2047 ] = 0xAA};

static bool g_use_data_a = true;  // true: write A next, false: write B next
static int g_write_count = 0;

void PrintBuffer(const char *header, unsigned char* buffer, uint32_t len)
{
	register uint32_t i = 0;
	printf("%s : %d\n", header, len);
	for (i = 0; i < len; i++) {
		if (i != 0 && i % 16 == 0) {
			printf("\n");
		}
		printf(" %02X", buffer[i]);
	}
	printf("\n");
}

/* Helper function: Initialize seclink */
static int ss_init(sl_ctx *sl_hnd)
{
	printf("SECLINK Initialize ...\n");
	if (SECLINK_OK != sl_init(sl_hnd)) {
		printf("   ssFail! sl_init\n");
		return -1;
	}
	printf("   OK\n\n");
	return 0;
}

/* Helper function: Deinitialize seclink */
static void ss_deinit(sl_ctx sl_hnd)
{
	printf("SECLINK Deinitialize ...\n");
	sl_deinit(sl_hnd);
	printf("   OK\n\n");
}

/* Helper function: Allocate output buffer */
static int ss_alloc_buffer(hal_data *output)
{
	output->data = (unsigned char *)malloc(TEST_SS_MAX_DATA_SIZE);
	if (output->data == NULL) {
		printf("   ssFail! malloc\n");
		return -1;
	}
	output->data_len = TEST_SS_MAX_DATA_SIZE;
	memset(output->data, 0, TEST_SS_MAX_DATA_SIZE);
	return 0;
}

/* Helper function: Free output buffer */
static void ss_free_buffer(hal_data *output)
{
	if (output->data != NULL) {
		free(output->data);
		output->data = NULL;
		output->data_len = 0;
	}
}

/* Helper function: Write data to slot */
static int ss_write(sl_ctx sl_hnd, const unsigned char *data, uint32_t data_len)
{
	hal_data input = {(unsigned char *)data, data_len};
	int ret = sl_write_storage(sl_hnd, TEST_SS_SLOT_INDEX, &input);
	if (ret == SECLINK_OK) {
		printf("   Successfully wrote %d bytes\n", data_len);
	} else {
		printf("   ssFail! sl_write_storage returned %d\n", ret);
	}
	return ret;
}

/* Helper function: Read data from slot */
static int ss_read(sl_ctx sl_hnd, hal_data *output)
{
	int ret = sl_read_storage(sl_hnd, TEST_SS_SLOT_INDEX, output);
	if (ret == SECLINK_OK) {
		printf("   Read %d bytes\n", output->data_len);
	} else if (ret == SECLINK_EMPTY_SLOT) {
		printf("   Slot is empty\n");
	} else {
		printf("   ssFail! sl_read_storage returned %d\n", ret);
	}
	return ret;
}

/* Helper function: Delete slot */
static int ss_delete(sl_ctx sl_hnd)
{
	int ret = sl_delete_storage(sl_hnd, TEST_SS_SLOT_INDEX);
	if (ret == SECLINK_OK) {
		printf("   Successfully deleted slot %d\n", TEST_SS_SLOT_INDEX);
	} else {
		printf("   ssFail! sl_delete_storage returned %d\n", ret);
	}
	return ret;
}

/* Helper function: Check if data matches A or B */
static int ss_check_data_type(const unsigned char *data, uint32_t len)
{
	if (len != TEST_SS_DATA_SIZE) {
		return -1;  // Unknown
	}
	if (memcmp(data, DATA_A, len) == 0) {
		return 0;  // Data A
	}
	if (memcmp(data, DATA_B, len) == 0) {
		return 1;  // Data B
	}
	return -1;  // Unknown
}

/* Check initial state and determine next data to write */
static void ss_check_initial_state(void)
{
	sl_ctx sl_hnd;
	hal_data output = {NULL, 0};
	int data_type;

	printf("=== Initial State Check ===\n\n");

	/* Initialize seclink */
	if (ss_init(&sl_hnd) < 0) {
		return;
	}

	printf("Read slot %d (initial state)...\n", TEST_SS_SLOT_INDEX);
	if (ss_alloc_buffer(&output) < 0) {
		ss_deinit(sl_hnd);
		return;
	}

	if (ss_read(sl_hnd, &output) == SECLINK_OK) {
		data_type = ss_check_data_type(output.data, output.data_len);
		if (data_type == 0) {
			printf("   Current data is: DATA_A\n");
			g_use_data_a = false;  // Next write should be B
		} else if (data_type == 1) {
			printf("   Current data is: DATA_B\n");
			g_use_data_a = true;   // Next write should be A
		} else {
			printf("   ssFail! Current data is: UNKNOWN\n");
			PrintBuffer("   Read data", output.data, output.data_len);
			g_use_data_a = true;
		}
	} else {
		g_use_data_a = true;  // Start with Data A
	}

	printf("\n   Next write will be: DATA_%c\n\n", g_use_data_a ? 'A' : 'B');
	ss_free_buffer(&output);
	ss_deinit(sl_hnd);
}

/* Run single test iteration: write -> read -> verify -> delete -> verify deletion */
static void ss_run_test_iteration(void)
{
	sl_ctx sl_hnd;
	hal_data output = {NULL, 0};
	const unsigned char *write_data = g_use_data_a ? DATA_A : DATA_B;
	int ret;

	printf("=== Write Test #%d (DATA_%c) ===\n\n", g_write_count, g_use_data_a ? 'A' : 'B');

	/* Initialize seclink */
	if (ss_init(&sl_hnd) < 0) {
		return;
	}

	/* Write data */
	printf("Write DATA_%c to slot %d...\n", g_use_data_a ? 'A' : 'B', TEST_SS_SLOT_INDEX);
	if (ss_write(sl_hnd, write_data, TEST_SS_DATA_SIZE) != SECLINK_OK) {
		ss_deinit(sl_hnd);
		return;
	}
	printf("   OK\n\n");

	/* Read back and verify */
	printf("Read back from slot %d...\n", TEST_SS_SLOT_INDEX);
	if (ss_alloc_buffer(&output) < 0) {
		ss_deinit(sl_hnd);
		return;
	}

	ret = ss_read(sl_hnd, &output);
	if (ret == SECLINK_OK) {
		if (output.data_len == TEST_SS_DATA_SIZE && 
		    memcmp(output.data, write_data, TEST_SS_DATA_SIZE) == 0) {
			printf("   Data verification: OK (DATA_%c)\n", g_use_data_a ? 'A' : 'B');
		} else {
			printf("   ssFail! Data verification: MISMATCH!\n");
			PrintBuffer("   Read data", output.data, output.data_len);
			
			/* Read again to verify */
			printf("\n   Re-reading slot %d...\n", TEST_SS_SLOT_INDEX);
			ss_free_buffer(&output);
			if (ss_alloc_buffer(&output) == 0) {
				ret = ss_read(sl_hnd, &output);
				if (ret == SECLINK_OK) {
					if (output.data_len == TEST_SS_DATA_SIZE && 
					    memcmp(output.data, write_data, TEST_SS_DATA_SIZE) == 0) {
						printf("   Re-read verification: OK (DATA_%c)\n", g_use_data_a ? 'A' : 'B');
					} else {
						printf("   ssFail! Re-read verification: MISMATCH again!\n");
						PrintBuffer("   Re-read data", output.data, output.data_len);
					}
				} else {
					printf("   ssFail! Re-read failed\n");
				}
			}
		}
	}
	ss_free_buffer(&output);
	printf("\n");

	/* Delete data */
	printf("Delete slot %d...\n", TEST_SS_SLOT_INDEX);
	ss_delete(sl_hnd);
	printf("   OK\n\n");

	/* Verify deletion */
	printf("Read slot %d (verify deletion)...\n", TEST_SS_SLOT_INDEX);
	if (ss_alloc_buffer(&output) < 0) {
		ss_deinit(sl_hnd);
		return;
	}
	ret = ss_read(sl_hnd, &output);
	if (ret == SECLINK_EMPTY_SLOT) {
		printf("   Slot %d is empty - deletion verified\n", TEST_SS_SLOT_INDEX);
	} else if (ret == SECLINK_OK) {
		printf("   ssFail! Data still exists in slot %d (unexpected)\n", TEST_SS_SLOT_INDEX);
	}
	ss_free_buffer(&output);
	printf("   OK\n\n");

	/* Toggle for next write */
	g_use_data_a = !g_use_data_a;
	g_write_count++;

	ss_deinit(sl_hnd);
}

void
test_securestorage(void)
{
	printf("\n========================================\n");
	printf("  Secure Storage Test\n");
	printf("========================================\n\n");

	/* Check initial state */
	ss_check_initial_state();

	/* Run test iterations */
	for (;;) {
		ss_run_test_iteration();
	}
}


static void display_test_scenario(void)
{
	printf("\nSelect Test Scenario.\n");
#ifdef CONFIG_EXAMPLES_MESSAGING_TEST
	printf("\t-Press M or m : Messaging F/W Test\n");	
#endif
#ifdef CONFIG_EXAMPLES_RECOVERY_TEST
	printf("\t-Press R or r : Recovery Test\n");
#endif
#ifdef CONFIG_EXAMPLES_BINARY_UPDATE_TEST
	printf("\t-Press U or u : Binary Update Test (All Tests)\n");
	printf("\t-Press S or s : Same Version Test\n");
	printf("\t-Press N or n : New Version Test\n");
	printf("\t-Press I or i : Invalid Binary Test\n");
#endif
	printf("\t-Press X or x : Terminate Tests.\n");
}

extern int preapp_start(int argc, char **argv);

#ifdef CONFIG_EXAMPLES_SMARTFS_POWERCUT
extern int smartfs_powercut_main(int argc, char *argv[]);
#endif

#ifdef CONFIG_APP_BINARY_SEPARATION
int main(int argc, char **argv)
#else
int wifiapp_main(int argc, char **argv)
#endif
{
#ifdef CONFIG_EXAMPLES_LOADABLE_MANUAL_TEST
	int ch;
	bool is_testing = true;
#endif

#if defined(CONFIG_SYSTEM_PREAPP_INIT) && defined(CONFIG_APP_BINARY_SEPARATION)
	preapp_start(argc, argv);
#endif

	printf("This is WIFI App\n");

#if defined(CONFIG_BINARY_MANAGER) && !defined(CONFIG_EXAMPLES_MICOM_TIMER_TEST)
	int ret;
	ret = binary_manager_notify_binary_started();
	if (ret < 0) {
		printf("WIFI notify 'START' state FAIL\n");
	}
#endif

#ifdef CONFIG_EXAMPLES_SMARTFS_POWERCUT
	smartfs_powercut_main(0, NULL);
#endif

#ifdef CONFIG_EXAMPLES_LOADABLE_MANUAL_TEST
	while (is_testing) {
		display_test_scenario();
		ch = getchar();
		switch (ch) {
#ifdef CONFIG_EXAMPLES_MESSAGING_TEST
		case 'M':
		case 'm':
			messaging_test();
			break;
#endif
#ifdef CONFIG_EXAMPLES_RECOVERY_TEST
		case 'R':
		case 'r':
			recovery_test();
			is_testing = false;
			break;
#endif
#ifdef CONFIG_EXAMPLES_BINARY_UPDATE_TEST
		case 'U':
		case 'u':
			binary_update_test_with_type("all_tests");
			break;
		case 'S':
		case 's':
			binary_update_test_with_type("same_version");
			break;
		case 'N':
		case 'n':
			binary_update_test_with_type("new_version");
			break;
		case 'I':
		case 'i':
			binary_update_test_with_type("invalid_binary");
			break;
#endif
		case 'X':
		case 'x':
			printf("Test will be finished.\n");
			is_testing = false;
			break;
		default:
			printf("Invalid Scenario.\n");
			break;
		}
	}

#elif defined(CONFIG_EXAMPLES_LOADABLE_AUTOMATIC_TEST)
#ifdef CONFIG_EXAMPLES_RECOVERY_AGING_TEST
	recovery_test();
#elif defined(CONFIG_EXAMPLES_UPDATE_AGING_TEST)
	binary_update_aging_test();
#endif
#endif
	
	task_create("secure_storage_test", 100, 4096, test_securestorage, NULL);

	while (1) {
		sleep(300);
		printf("[%d] WIFI ALIVE\n", getpid());
	}
	return 0;
}