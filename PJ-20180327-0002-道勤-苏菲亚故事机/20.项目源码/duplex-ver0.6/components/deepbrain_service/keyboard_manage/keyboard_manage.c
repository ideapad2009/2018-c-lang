#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "keyboard_manage.h"
#include "mcu_serial_comm.h"
#include "driver/adc.h"
#include "device_api.h"
#include "esp_log.h"
#include "driver/adc1_vref_cali.h"
#include "led.h"
#include "keyboard_event.h"
#include "keyboard_service.h"

//#define PRINT_ADC_VALUE     
#define KEY_SCAN_PERIOD_MS	40
#define KEY_SHORT_PRESS_MS 	800				// 200 ms
#define KEY_LONG_PRESS_MS 	(1*1000)  		// 2 seconds
#define KEY_MATRIX_TAG 		"key matrix"
#define HIT_COUNT           2

//store adc key run value 
typedef struct
{
	int 		   adc_value;	//current adc value
	DEVICE_KEY_EVENT_T curr_key_num;//current key num
	DEVICE_KEY_EVENT_T last_key_num;//last key num
	unsigned int   key_hold_ms;	//key press hold time
	unsigned int   key_hold_flag;
	key_matrix_cb  cb;
	adc1_channel_t channel;
}KEY_MATRIX_STATUS_T;

typedef struct {
    TickType_t interval;
    TimerHandle_t tmr;
	KEY_MATRIX_STATUS_T key1_status;//adc key1
	battery_cb bat_cb;
} key_matrix_t;

KEYBOARD_OBJ_T *g_keyboard_1_obj = NULL;
KEYBOARD_OBJ_T *g_keyboard_2_obj = NULL;

static void adc_battery_process(battery_cb bat_cb)
{
	int adc_value = 0;
	static int total_count = 0;
	static int total_value = 0;
	
	 if (esp_efuse_vref_is_cali() == 2) 
	{
		adc_value = get_adc_voltage_tp(ADC_ATTEN_11db, ADC1_CHANNEL_6);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,TWO Point adc:%d\r\n", ADC1_CHANNEL_6, adc_value);
#endif
    } 
	else if (esp_efuse_vref_is_cali() == 1) 
	{
		adc_value = get_adc_voltage_deepbrain(ADC_ATTEN_11db, ADC1_CHANNEL_6);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,Vref adc:%d\r\n", ADC1_CHANNEL_6, adc_value);
#endif
	} 
	else 
	{
		adc_value = adc1_get_voltage(ADC1_CHANNEL_6);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,No calibration data, Raw:%d\r\n", ADC1_CHANNEL_6, adc_value);
#endif
	}

	total_count++;
	total_value += adc_value;
	if (total_count >= 50)
	{
		if (bat_cb)
		{
			bat_cb(total_value*2.0/total_count/1000);
		}
		total_count = 0;
		total_value = 0;
	}
}

static void adc_key_matrix_run(xTimerHandle tmr)
{
	int adc_1_value;
	int adc_2_value;
	KEY_EVENT_T key_1_event;
	KEY_EVENT_T key_2_event;

	key_matrix_t* key_matrix = (key_matrix_t*) pvTimerGetTimerID(tmr);

	if (esp_efuse_vref_is_cali() == 2) 
	{
		adc_1_value = get_adc_voltage_tp(ADC_ATTEN_11db, ADC1_CHANNEL_3);
		adc_2_value = get_adc_voltage_tp(ADC_ATTEN_11db, ADC1_CHANNEL_0);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,TWO Point adc:%d\r\nchannel %d,TWO Point adc:%d\r\n", ADC1_CHANNEL_3, adc_1_value, ADC1_CHANNEL_0, adc_2_value);
#endif
    } 
	else if (esp_efuse_vref_is_cali() == 1) 
	{
		adc_1_value = get_adc_voltage_deepbrain(ADC_ATTEN_11db, ADC1_CHANNEL_3);
		adc_2_value = get_adc_voltage_deepbrain(ADC_ATTEN_11db, ADC1_CHANNEL_0);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,Vref adc:%d\r\nchannel %d,Vref adc:%d\r\n", ADC1_CHANNEL_3, adc_1_value, ADC1_CHANNEL_0, adc_2_value);
#endif
	} 
	else 
	{
		adc_1_value = adc1_get_voltage(ADC1_CHANNEL_3);
		adc_2_value = adc1_get_voltage(ADC1_CHANNEL_0);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,No calibration data, Raw:%d\r\nchannel %d,No calibration data, Raw:%d\r\n", ADC1_CHANNEL_3, adc_1_value, ADC1_CHANNEL_3, adc_1_value);
#endif
	}

	key_1_event = adc_keyboard_process(g_keyboard_1_obj, adc_1_value);
	key_2_event = adc_keyboard_process(g_keyboard_2_obj, adc_2_value);

	if (key_1_event != 0 || key_2_event != 0)
	{
		if (key_1_event != 0)
		{
			key_matrix_callback(key_1_event);
		}
		else
		{
			key_matrix_callback(key_2_event);
		}
	}

	adc_battery_process(key_matrix->bat_cb);

	xTimerStart(key_matrix->tmr, 40/portTICK_PERIOD_MS);

	return;
}

static void init_key_status(KEY_MATRIX_STATUS_T *status, key_matrix_cb cb, adc1_channel_t channel)
{
	if (status == NULL)
	{
		return;
	}

	status->curr_key_num = DEVICE_KEY_START;
	status->last_key_num = DEVICE_KEY_START;
	status->cb = cb;
	status->channel = channel;

	esp_adc1_cali_config(ADC_WIDTH_12Bit, ADC_ATTEN_11db, channel);
}

void keyboard_1_init(void)
{
	g_keyboard_1_obj = keyboard_create(4);
	if (g_keyboard_1_obj == NULL) {
		ESP_LOGE("KEYBOARD", "keyboard_create fail !");
		return;
	}

	KEYBOARD_CFG_T keyboard_cfg = {
		.key_num        = 4,   //按键数量	
		.fault_tol      = 80, //容错值
		.scan_cycle     = 40 , //扫描周期ms
		.longpress_time = 800, //长按时间ms
		
		.keyboard[0] = {//空闲状态
			.adc_value = 2260,
		},
		.keyboard[1] = {//按键K5
			.adc_value   = 1150,
			.sigleclick  = KEY_EVENT_NEXT,
			.longpress   = KEY_EVENT_VOL_UP,
		},
		.keyboard[2] = {//按键K6
			.adc_value  = 210,
			.sigleclick  = KEY_EVENT_OTHER_KEY_CHILD_LOCK_PLAY,
			.longpress  = KEY_EVENT_CHILD_LOCK,
		},
		.keyboard[3] = {//按键K7
			.adc_value   = 540,
			.longpress   = KEY_EVENT_WIFI,
			.sigleclick  = KEY_EVENT_SDCARD,
		},
		.keyboard[4] = {//按键K8
			.adc_value   = 850,
			.sigleclick  = KEY_EVENT_COLOR_LED,
			.longpress   = KEY_EVENT_OTHER_KEY_CHILD_LOCK_PLAY,
		},
	};

	g_keyboard_1_obj->k_cfg = keyboard_cfg;
}

void keyboard_2_init(void)
{
	g_keyboard_2_obj = keyboard_create(4);
	if (g_keyboard_2_obj == NULL) {
		ESP_LOGE("KEYBOARD", "keyboard_create fail !");
		return;
	}

	KEYBOARD_CFG_T keyboard_cfg = {
		.key_num        = 4,   //按键数量	
		.fault_tol      = 80, //容错值
		.scan_cycle     = 40 , //扫描周期ms
		.longpress_time = 800, //长按时间ms
		
		.keyboard[0] = {//空闲状态
			.adc_value = 2269,
		},
		.keyboard[1] = {//按键K1
			.adc_value   = 215,
			.sigleclick  = KEY_EVENT_CHAT_START,
			.longpress   = KEY_EVENT_OTHER_KEY_CHILD_LOCK_PLAY,
		},
		.keyboard[2] = {//按键K2
			.adc_value   = 545,
			.sigleclick  = KEY_EVENT_PUSH_PLAY,
			.longpress   = KEY_EVENT_OTHER_KEY_CHILD_LOCK_PLAY,
		},
		.keyboard[3] = {//按键K3
			.adc_value   = 859,
			.push        = KEY_EVENT_WECHAT_START,
			.release     = KEY_EVENT_WECHAT_STOP,
		},
		.keyboard[4] = {//按键K4
			.adc_value   = 1155,
			.sigleclick  = KEY_EVENT_PREV,
			.longpress   = KEY_EVENT_VOL_DOWN,
		},
	};

	g_keyboard_2_obj->k_cfg = keyboard_cfg;
}

key_matrix_handle_t adc_keyboard_matrix_init(key_matrix_cb _cb, battery_cb bat_cb)
{
	key_matrix_t* key_matrix = NULL;	

	key_matrix = (key_matrix_t*) esp32_malloc(sizeof(key_matrix_t));
	if (key_matrix == NULL)
	{
		return NULL;
	}
	
	memset((char*)key_matrix, 0, sizeof(key_matrix_t));
	key_matrix->bat_cb = bat_cb;
	//init_key_status(&key_matrix->key1_status, _cb, ADC1_CHANNEL_3);
	esp_adc1_cali_config(ADC_WIDTH_12Bit, ADC_ATTEN_11db, ADC1_CHANNEL_6);

	keyboard_1_init();
	keyboard_2_init();
	
	key_matrix->tmr = xTimerCreate((const char*)"tmr_keyborad_matrix", KEY_SCAN_PERIOD_MS/portTICK_PERIOD_MS, pdFALSE, key_matrix,
                adc_key_matrix_run);

	xTimerStart(key_matrix->tmr, 40/portTICK_PERIOD_MS);

	return key_matrix;
}


void adc_keyboard_matrix_free(key_matrix_handle_t _handle)
{
	key_matrix_t * handle = (key_matrix_t *)_handle;

	if (handle == NULL)
	{
		return;
	}

	if (handle->tmr != NULL)
	{
	    xTimerStop(handle->tmr, portMAX_DELAY);
	    xTimerDelete(handle->tmr, portMAX_DELAY);
	    handle->tmr = NULL;
	}

	esp32_free(handle);

	return;
}


