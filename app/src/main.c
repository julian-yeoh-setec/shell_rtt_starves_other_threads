/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h> 
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <settings/settings.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#define LOG_MODULE_NAME main
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);


#define TEST_SERVICE_128BIT_UUID 		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0))
#define TEST_CHARACTERISTIC_128BIT_UUID 	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1))

struct bt_gatt_attr gatt_attributes[] =
{
	BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_128BIT_UUID),		

	BT_GATT_CHARACTERISTIC(TEST_CHARACTERISTIC_128BIT_UUID,	
	BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY, 
	BT_GATT_PERM_WRITE,
	NULL, 
	NULL, 
	NULL),

	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), 	
};

static struct bt_gatt_service test_service = BT_GATT_SERVICE(gatt_attributes);

static void bluetooth_initialize(void)
{
	int ret;

	ret = bt_enable(NULL);

	if (ret) {
		LOG_ERR("Bluetooth init failed (err %d)\n", ret);
		return;
	}

	ret = bt_gatt_service_register(&test_service);	

	if (ret) {
		LOG_ERR("GATT service register failed (err %d)\n", ret);
		return;
	}	
}	


#define WORKQUEUE_THREAD_PRIO			14  /* Same priority as thread shell_rtt*/
K_THREAD_STACK_DEFINE(workqueue_thread_stack_area, 2048);

static struct k_work_q work_q;
static struct k_work_delayable delayable_work;
static struct k_work_queue_config queue_config = { "queue_1", true };

static void work_handler(struct k_work *work)
{
	int ret;

	ret = k_work_schedule_for_queue(&work_q, &delayable_work, K_SECONDS(1));
	if (ret < 0) {
		LOG_ERR("Cannot submit work: %d", ret);
	}
}

static void workqueue_thread_init(void)
{
	int ret;

	k_work_queue_start(&work_q, workqueue_thread_stack_area,
	               K_THREAD_STACK_SIZEOF(workqueue_thread_stack_area), WORKQUEUE_THREAD_PRIO, &queue_config);

	k_work_init_delayable(&delayable_work, work_handler);

	ret = k_work_schedule_for_queue(&work_q, &delayable_work, K_SECONDS(1));
	if (ret < 0) {
		LOG_ERR("Cannot submit work: %d", ret);
		
	}
}


void main(void)
{
	int ret;

	bluetooth_initialize();
	workqueue_thread_init();

	while (true) {
		static uint32_t seconds = 0;

		k_sleep(K_MSEC(1000));
		seconds++;	
		LOG_INF("main thread alive %02u:%02u:%02u", seconds / 60 / 60 , seconds / 60, seconds % 60 );
		
		ret = bt_gatt_notify(NULL, &gatt_attributes[1], &seconds, sizeof(seconds));
		if (ret < 0) {
			LOG_ERR("Cannot notify: %d", ret);
			
		}		
	}	
}
 