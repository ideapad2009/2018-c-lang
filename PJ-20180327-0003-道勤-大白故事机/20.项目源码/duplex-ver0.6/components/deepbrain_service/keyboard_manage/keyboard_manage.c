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

//#define PRINT_ADC_VALUE     
#define KEY_SCAN_PERIOD_MS	100
#define KEY_SHORT_PRESS_MS 	500//800				// 200 ms
#define KEY_LONG_PRESS_MS 	(1*1000)  		// 2 seconds
#define KEY_MATRIX_TAG 		"key matrix"
#define HIT_COUNT           1

//store adc key run value 
typedef struct
{
	int 		   adc_value;	//current adc value
	DEVICE_NOTIFY_KEY_T curr_key_num;//current key num
	DEVICE_NOTIFY_KEY_T last_key_num;//last key num
	unsigned int   key_hold_ms;	//key press hold time
	unsigned int   key_hold_flag;
	key_matrix_cb  cb;
	adc1_channel_t channel;
}KEY_MATRIX_STATUS_T;

typedef struct {
    TickType_t interval;
    TimerHandle_t tmr;
	KEY_MATRIX_STATUS_T key1_status;//adc key1
	KEY_MATRIX_STATUS_T key2_status;//adc key2
	battery_cb bat_cb;
} key_matrix_t;

//channe 0
#define KEY_MATRIX_IDEL_VOLTAGE		    (2212)     //空闲
#define KEY_MATRIX_K1_VOLTAGE(x)	    (177 + x) //
#define KEY_MATRIX_K2_VOLTAGE(x)	    (505 + x) //
#define KEY_MATRIX_K3_VOLTAGE(x)   		(811 + x) //
#define KEY_MATRIX_K4_VOLTAGE(x)	    (1115 + x) //
#define KEY_MATRIX_K5_VOLTAGE(x)	    (1428 + x)  //

//channe 3
#define KEY_MATRIX_K6_VOLTAGE(x)	    (177 + x)  //
#define KEY_MATRIX_K7_VOLTAGE(x)	    (506 + x)  //
#define KEY_MATRIX_K8_VOLTAGE(x)	    (812 + x)  //
#define KEY_MATRIX_K9_VOLTAGE(x)	    (1120 + x)  //
#define KEY_MATRIX_K10_VOLTAGE(x)	    (1427 + x)  //
#define KEY_MATRIX_VOLTAGE_ERROR 		(100)		//电压容错值

#define KEY_MATRIX_VOLTAGE_RANGE(a,b) ((a >= ((b)-KEY_MATRIX_VOLTAGE_ERROR)) && (a <= ((b)+KEY_MATRIX_VOLTAGE_ERROR)))

static void adc_key1_process(KEY_MATRIX_STATUS_T *status)
{
	if (status == NULL)
		return;

    if (esp_efuse_vref_is_cali() == 2) 
	{
		status->adc_value = get_adc_voltage_tp(ADC_ATTEN_11db, status->channel);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,TWO Point adc:%d\r\n", status->channel, status->adc_value);
#endif
    } 
	else if (esp_efuse_vref_is_cali() == 1) 
	{
		status->adc_value = get_adc_voltage_deepbrain(ADC_ATTEN_11db, status->channel);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,Vref adc:%d\r\n", status->channel, status->adc_value);
#endif
	} 
	else 
	{
		status->adc_value = adc1_get_voltage(status->channel);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,No calibration data, Raw:%d\r\n", status->channel, status->adc_value);
#endif
	}

	static int key_count[6] = {0};
	static int offset_value  = 0;
	static int idel_value    = 0;
	static int sum_of_idel  = 0;
	static int num = 0;

#ifdef PRINT_ADC_VALUE
		printf("offset_value = %d\n", offset_value);
#endif

	if ((status->adc_value >= 2080) && (status->adc_value <= 2400))
	{
		num++;
		sum_of_idel += status->adc_value;
	
		status->curr_key_num = DEVICE_NOTIFY_KEY_START;	
		if (num == 100) {
			idel_value   = sum_of_idel / num;
			offset_value = idel_value - KEY_MATRIX_IDEL_VOLTAGE;
			sum_of_idel  = 0;
			num = 0;
		}
		
	}
/*
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K1_VOLTAGE(offset_value)))
	{
		key_count[1]++;
		if (key_count[1] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_CHAT_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K2_VOLTAGE(offset_value)))
	{
		key_count[2]++;
		if (key_count[2] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_LED_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K3_VOLTAGE(offset_value)))
	{
		key_count[3]++;
		if (key_count[3] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_VOL_UP_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K4_VOLTAGE(offset_value)))
	{
		key_count[4]++;
		if (key_count[4] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_VOL_DOWN_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K5_VOLTAGE(offset_value)))
	{
		key_count[1]++;
		if (key_count[1] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_MENU_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
*/
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K6_VOLTAGE(offset_value)))
	{
		key_count[1]++;
		if (key_count[1] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_K6_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K7_VOLTAGE(offset_value)))
	{
		key_count[2]++;
		if (key_count[2] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_K7_TAP;
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K8_VOLTAGE(offset_value)))
	{
		key_count[3]++;
		if (key_count[3] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_K8_TAP;
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K9_VOLTAGE(offset_value)))
	{
		key_count[4]++;
		if (key_count[4] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_K9_TAP;
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K10_VOLTAGE(offset_value)))
	{
		key_count[5]++;
		if (key_count[5] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_K10_TAP;
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else
	{
		return;
	}

	status->key_hold_ms += KEY_SCAN_PERIOD_MS;
	if (status->curr_key_num == status->last_key_num)
	{
		if (status->curr_key_num != DEVICE_NOTIFY_KEY_START)
		{
			if (status->key_hold_ms >= KEY_LONG_PRESS_MS && status->key_hold_flag == 0)
			{
				status->cb(status->last_key_num + 3);
				status->key_hold_flag = 1;
			}
		}
	}
	else
	{	
		switch (status->curr_key_num)
		{
			case DEVICE_NOTIFY_KEY_START:
			{
				status->cb(status->last_key_num + 2);
				//key release
				if (status->key_hold_ms <= KEY_SHORT_PRESS_MS)
				{
					status->cb(status->last_key_num);
				}
				else if (status->key_hold_ms >= KEY_LONG_PRESS_MS && status->key_hold_flag != 1)
				{
					status->cb(status->last_key_num + 3);
				}
				status->key_hold_flag = 0;
				break;
			}
			case DEVICE_NOTIFY_KEY_K6_TAP ... DEVICE_NOTIFY_KEY_K10_TAP:
			{
				//key press
				status->cb(status->curr_key_num + 1);
				break;
			}
			default:
				break;
		}
		status->key_hold_ms = 0;
		status->last_key_num = status->curr_key_num;
	}
}

static void adc_key2_process(KEY_MATRIX_STATUS_T *status)
{
	if (status == NULL)
		return;

    if (esp_efuse_vref_is_cali() == 2) 
	{
		status->adc_value = get_adc_voltage_tp(ADC_ATTEN_11db, status->channel);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,TWO Point adc:%d\r\n", status->channel, status->adc_value);
#endif
    } 
	else if (esp_efuse_vref_is_cali() == 1) 
	{
		status->adc_value = get_adc_voltage_deepbrain(ADC_ATTEN_11db, status->channel);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,Vref adc:%d\r\n", status->channel, status->adc_value);
#endif
	} 
	else 
	{
		status->adc_value = adc1_get_voltage(status->channel);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,No calibration data, Raw:%d\r\n", status->channel, status->adc_value);
#endif
	}

	static int key_count[6] = {0};
	static int offset_value  = 0;
	static int idel_value    = 0;
	static int sum_of_idel  = 0;
	static int num = 0;

#ifdef PRINT_ADC_VALUE
		printf("offset_value = %d\n", offset_value);
#endif

	if ((status->adc_value >= 2080) && (status->adc_value <= 2400))
	{
		num++;
		sum_of_idel += status->adc_value;
	
		status->curr_key_num = DEVICE_NOTIFY_KEY_START;	
		if (num == 100) {
			idel_value   = sum_of_idel / num;
			offset_value = idel_value - KEY_MATRIX_IDEL_VOLTAGE;
			sum_of_idel  = 0;
			num = 0;
		}
		
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K1_VOLTAGE(offset_value)))
	{
		key_count[1]++;
		if (key_count[1] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_K1_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K2_VOLTAGE(offset_value)))
	{
		key_count[2]++;
		if (key_count[2] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_K2_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K3_VOLTAGE(offset_value)))
	{
		key_count[3]++;
		if (key_count[3] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_K3_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K4_VOLTAGE(offset_value)))
	{
		key_count[4]++;
		if (key_count[4] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_K4_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}

	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K5_VOLTAGE(offset_value)))
	{
		key_count[5]++;
		if (key_count[5] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_K5_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
/*
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K6_VOLTAGE(offset_value)))
	{
		key_count[6]++;
		if (key_count[6] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_PREV_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K7_VOLTAGE(offset_value)))
	{
		key_count[7]++;
		if (key_count[7] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_PLAY_TAP;
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K8_VOLTAGE(offset_value)))
	{
		key_count[8]++;
		if (key_count[8] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_NEXT_TAP;
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K9_VOLTAGE(offset_value)))
	{
		key_count[9]++;
		if (key_count[9] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_STORY_TAP;
			memset(key_count, 0, sizeof(key_count));
		}
	}
*/
	else
	{
		return;
	}
	
	status->key_hold_ms += KEY_SCAN_PERIOD_MS;
	if (status->curr_key_num == status->last_key_num)
	{
		if (status->curr_key_num != DEVICE_NOTIFY_KEY_START)
		{
			if (status->key_hold_ms >= KEY_LONG_PRESS_MS && status->key_hold_flag == 0)
			{
				status->cb(status->last_key_num + 3);
				status->key_hold_flag = 1;
			}
		}
	}
	else
	{	
		switch (status->curr_key_num)
		{
			case DEVICE_NOTIFY_KEY_START:
			{
				status->cb(status->last_key_num + 2);
				//key release
				if (status->key_hold_ms <= KEY_SHORT_PRESS_MS)
				{
					status->cb(status->last_key_num);
				}
				else if (status->key_hold_ms >= KEY_LONG_PRESS_MS && status->key_hold_flag != 1)
				{
					status->cb(status->last_key_num + 3);
				}
				status->key_hold_flag = 0;
				break;
			}
			case DEVICE_NOTIFY_KEY_K1_TAP ... DEVICE_NOTIFY_KEY_K5_TAP:
			{
				//key press
				status->cb(status->curr_key_num + 1);
				break;
			}
			default:
				break;
		}
		status->key_hold_ms = 0;
		status->last_key_num = status->curr_key_num;
	}
}

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
	key_matrix_t* key_matrix = (key_matrix_t*) pvTimerGetTimerID(tmr);
	
	adc_key1_process(&key_matrix->key1_status);//ADC1_CHANNEL_3
	adc_key2_process(&key_matrix->key2_status);//ADC1_CHANNEL_0
	
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

	status->curr_key_num = DEVICE_NOTIFY_KEY_START;
	status->last_key_num = DEVICE_NOTIFY_KEY_START;
	status->cb = cb;
	status->channel = channel;

	esp_adc1_cali_config(ADC_WIDTH_12Bit, ADC_ATTEN_11db, channel);
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
	init_key_status(&key_matrix->key1_status, _cb, ADC1_CHANNEL_3);
	init_key_status(&key_matrix->key2_status, _cb, ADC1_CHANNEL_0);
	esp_adc1_cali_config(ADC_WIDTH_12Bit, ADC_ATTEN_11db, ADC1_CHANNEL_6);
	
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


