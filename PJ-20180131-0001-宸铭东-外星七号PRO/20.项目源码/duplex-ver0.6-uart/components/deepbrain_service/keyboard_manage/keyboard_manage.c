
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "keyboard_manage.h"
#include "driver/adc.h"
#include "device_api.h"
#include "esp_log.h"
#include "driver/adc1_vref_cali.h"

//#define PRINT_ADC_VALUE     
#define KEY_SCAN_PERIOD_MS	30
#define KEY_SHORT_PRESS_MS 	100				// 100 ms
#define KEY_DOUBLE_PRESS_MS 500				// 500 ms
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

#define KEY_MATRIX_IDEL_VOLTAGE			(2475)
#define KEY_MATRIX_CHAT_VOLTAGE(x)		(305 + x)	
#define KEY_MATRIX_PREV_VOLTAGE(x)		(760 + x)
#define KEY_MATRIX_PLAY_VOLTAGE(x)		(1415 + x)
#define KEY_MATRIX_NEXT_VOLTAGE(x)		(1655 + x)
#define KEY_MATRIX_WECHAT_VOLTAGE(x)	(1170 + x)
#define KEY_MATRIX_VOLTAGE_ERROR 		(50)  		//电压容错值

#define KEY_MATRIX_VOLTAGE_RANGE(a,b) ((a >= ((b)-KEY_MATRIX_VOLTAGE_ERROR)) && (a <= ((b)+KEY_MATRIX_VOLTAGE_ERROR)))

#if 0
static void adc_key1_process(KEY_MATRIX_STATUS_T *status)
{
	if (status == NULL)
	{
		return;
	}
	
    if (esp_efuse_vref_is_cali() == 2) 
	{
		status->adc_value = get_adc_voltage_tp(ADC_ATTEN_11db, status->channel);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,TWO Point adc:%d\r\n", status->channel, status->adc_value);
#endif
    } 
	else if (esp_efuse_vref_is_cali() == 1) 
	{
		status->adc_value = get_adc_voltage_filter(ADC_ATTEN_11db, status->channel);
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

	static int key_count[8] = {0};
	static int offset_value  = 0;
	static int idel_value    = 0;
	static int sum_of_idel  = 0;
	static int num = 0;
	static uint64_t chat_press_time = 0;
	static uint64_t play_press_time = 0;
	static int is_double_press = 0;
	static int last_press_key = -1;
	static uint64_t last_press_time = 0;

#ifdef PRINT_ADC_VALUE
		printf("offset_value = %d\n", offset_value);
#endif

	if ((last_press_key == DEVICE_KEY_CHAT_TAP 
		|| last_press_key == DEVICE_KEY_PLAY_TAP)
		&& last_press_time > 0)
	{
		if (abs(get_time_of_day() - last_press_time) > KEY_DOUBLE_PRESS_MS)
		{
			
			//printf("[%d],[%llu]\r\n", last_press_key, get_time_of_day() - last_press_time);
			status->cb(last_press_key);
			last_press_key = -1;
			last_press_time = 0;
		}
	}

	if ((status->adc_value >= 2080) && (status->adc_value <= KEY_MATRIX_IDEL_VOLTAGE))
	{
		num++;
		sum_of_idel += status->adc_value;
	
		status->curr_key_num = DEVICE_KEY_START;	
		if (num == 100) 
		{
			idel_value   = sum_of_idel / num;
			offset_value = idel_value - KEY_MATRIX_IDEL_VOLTAGE;
			sum_of_idel  = 0;
			num = 0;
		}
		offset_value = 0;
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_CHAT_VOLTAGE(offset_value)))
	{
		key_count[1]++;
		if (key_count[1] >= HIT_COUNT) 
		{
			status->curr_key_num = DEVICE_KEY_CHAT_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_PREV_VOLTAGE(offset_value)))
	{
		key_count[2]++;
		if (key_count[2] >= HIT_COUNT) 
		{
			status->curr_key_num = DEVICE_KEY_PREV_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_PLAY_VOLTAGE(offset_value)))
	{
		key_count[3]++;
		if (key_count[3] >= HIT_COUNT) 
		{
			status->curr_key_num = DEVICE_KEY_PLAY_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_NEXT_VOLTAGE(offset_value)))
	{
		key_count[4]++;
		if (key_count[4] >= HIT_COUNT) 
		{
			status->curr_key_num = DEVICE_KEY_NEXT_TAP; 
			memset(key_count, 0, sizeof(key_count));
		}
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_WECHAT_VOLTAGE(offset_value)))
	{
		key_count[5]++;
		if (key_count[5] >= HIT_COUNT) 
		{
			status->curr_key_num = DEVICE_KEY_WECHAT_TAP; 
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
		if (status->curr_key_num != DEVICE_KEY_START)
		{
			if (status->key_hold_ms >= KEY_LONG_PRESS_MS && status->key_hold_flag == 0)
			{
				status->cb(status->last_key_num + 3);
				status->key_hold_flag = 1;
			}
				
			if (status->curr_key_num == DEVICE_KEY_WECHAT_TAP
				&& status->key_hold_ms == 30*15)
			{
				status->cb(status->curr_key_num + 1);
			}
		}
	}
	else
	{	
		switch (status->curr_key_num)
		{
			case DEVICE_KEY_START:
			{
				status->cb(status->last_key_num + 2);
				//key release
				if (status->key_hold_ms < KEY_LONG_PRESS_MS)
				{
					if (is_double_press == 0)
					{
						if (status->last_key_num == DEVICE_KEY_CHAT_TAP
							|| status->last_key_num == DEVICE_KEY_PLAY_TAP)
						{
							last_press_key = status->last_key_num;
							last_press_time = get_time_of_day() - status->key_hold_ms;
							//printf("[%d],[%llu]\r\n", last_press_key, last_press_time);
						}
						else
						{
							status->cb(status->last_key_num);
						}
					}
					else
					{
						is_double_press = 0;
					}
				}
				else if (status->key_hold_ms >= KEY_LONG_PRESS_MS && status->key_hold_flag != 1)
				{
					status->cb(status->last_key_num + 3);
				}
				status->key_hold_flag = 0;
				break;
			}
			case DEVICE_KEY_CHAT_TAP:
			{
				//double click
				if (chat_press_time > 0)
				{	
					if (get_time_of_day() - chat_press_time <= KEY_DOUBLE_PRESS_MS)
					{
						status->cb(status->curr_key_num + 4);
						is_double_press = 1;
					}
					else
					{
						chat_press_time = get_time_of_day();
						status->cb(status->curr_key_num + 1);
					}
				}
				else
				{
					chat_press_time = get_time_of_day();
					status->cb(status->curr_key_num + 1);
				}
				last_press_key = -1;
				last_press_time = 0;
				break;
			}
			case DEVICE_KEY_PLAY_TAP:
			{
				//double click
				if (play_press_time > 0)
				{	
					if (get_time_of_day() - play_press_time <= KEY_DOUBLE_PRESS_MS)
					{
						status->cb(status->curr_key_num + 4);
						is_double_press = 1;
					}
					else
					{
						play_press_time = get_time_of_day();
						status->cb(status->curr_key_num + 1);
					}
				}
				else
				{
					play_press_time = get_time_of_day();
					status->cb(status->curr_key_num + 1);
				}
				last_press_key = -1;
				last_press_time = 0;
				break;
			}
			case DEVICE_KEY_PREV_TAP:
			case DEVICE_KEY_NEXT_TAP:
			{				
				last_press_key = -1;
				last_press_time = 0;
				status->cb(status->curr_key_num + 1);
				break;
			}	
			case DEVICE_KEY_WECHAT_TAP:
			{				
				last_press_key = -1;
				last_press_time = 0;
				break;
			}
			default:
				break;
		}
		status->key_hold_ms = 0;
		status->last_key_num = status->curr_key_num;
	}
}
#endif

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
		adc_value = get_adc_voltage_filter(ADC_ATTEN_11db, ADC1_CHANNEL_6);
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
	
//	adc_key1_process(&key_matrix->key1_status);

	adc_battery_process(key_matrix->bat_cb);

	xTimerStart(key_matrix->tmr, KEY_SCAN_PERIOD_MS/portTICK_PERIOD_MS);

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
	esp_adc1_cali_config(ADC_WIDTH_12Bit, ADC_ATTEN_11db, ADC1_CHANNEL_6);
	
	key_matrix->tmr = xTimerCreate((const char*)"tmr_keyborad_matrix", KEY_SCAN_PERIOD_MS/portTICK_PERIOD_MS, pdFALSE, key_matrix,
                adc_key_matrix_run);

	xTimerStart(key_matrix->tmr, KEY_SCAN_PERIOD_MS/portTICK_PERIOD_MS);

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


