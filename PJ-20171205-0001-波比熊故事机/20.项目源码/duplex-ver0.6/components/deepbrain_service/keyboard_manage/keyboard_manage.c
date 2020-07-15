
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
#include "MediaHal.h"

//#define PRINT_ADC_VALUE     
#define KEY_SCAN_PERIOD_MS	30
#define KEY_SHORT_PRESS_MS 	800				// 200 ms
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
	adc1_channel_t channel_0;//adc key2
	adc1_channel_t channel_3;//adc key1
	adc1_channel_t channel_5;//adc vol
}KEY_MATRIX_STATUS_T;

typedef struct {
    TickType_t interval;
    TimerHandle_t tmr;
	KEY_MATRIX_STATUS_T key1_status;//adc key1 ,channel 3
	KEY_MATRIX_STATUS_T key2_status;//adc key2 ,channel 0
	KEY_MATRIX_STATUS_T vol_num;//adc vol ,channel 5
	battery_cb bat_cb;
} key_matrix_t;

#define KEY_MATRIX_ADC_KEY_2_IDEL_VOLTAGE		    (2224)     //空闲 adc key2 ,channel 0
#define KEY_MATRIX_ADC_KEY_1_IDEL_VOLTAGE		    (2221)     //空闲 adc key1 ,channel 3
#define KEY_MATRIX_K1_VOLTAGE(x)	    			(189 + x) //
#define KEY_MATRIX_K2_VOLTAGE(x)	    			(521 + x) //
#define KEY_MATRIX_K3_VOLTAGE(x)   					(827 + x) //
#define KEY_MATRIX_K4_VOLTAGE(x)	    			(1126 + x) //
#define KEY_MATRIX_K5_VOLTAGE(x)	    			(1437 + x)  //
#define KEY_MATRIX_K6_VOLTAGE(x)	    			(192 + x)  //
#define KEY_MATRIX_K7_VOLTAGE(x)	    			(522 + x)  //
#define KEY_MATRIX_K8_VOLTAGE(x)	    			(831 + x)  //
#define KEY_MATRIX_K9_VOLTAGE(x)	    			(1125 + x)  //
#define KEY_MATRIX_K10_VOLTAGE(x)	    			(1444 + x)  //
#define KEY_MATRIX_K11_VOLTAGE(x)	    			(0 + x)  //
#define KEY_MATRIX_K12_VOLTAGE(x)	    			(1746 + x)  //
#define KEY_MATRIX_VOLTAGE_ERROR 					(50)	//电压容错值

#define KEY_MATRIX_VOLTAGE_RANGE(a,b) ((a >= ((b)-KEY_MATRIX_VOLTAGE_ERROR)) && (a <= ((b)+KEY_MATRIX_VOLTAGE_ERROR)))

static void adc_key1_process(KEY_MATRIX_STATUS_T *status)
{
	if (status == NULL)
	{
		return;
	}
	
    if (esp_efuse_vref_is_cali() == 2) 
	{
		status->adc_value = get_adc_voltage_tp(ADC_ATTEN_11db, status->channel_3);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,TWO Point adc:%d\r\n", status->channel_3, status->adc_value);
#endif
    } 
	else if (esp_efuse_vref_is_cali() == 1) 
	{
		status->adc_value = get_adc_voltage_filter(ADC_ATTEN_11db, status->channel_3);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,Vref adc:%d\r\n", status->channel_3, status->adc_value);
#endif
	} 
	else 
	{
		status->adc_value = adc1_get_voltage(status->channel_3);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,No calibration data, Raw:%d\r\n", status->channel_3, status->adc_value);
#endif
	}

	static int key_count[7] = {0};
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
			offset_value = idel_value - KEY_MATRIX_ADC_KEY_1_IDEL_VOLTAGE;
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
	else if (KEY_MATRIX_VOLTAGE_RANGE(status->adc_value, KEY_MATRIX_K12_VOLTAGE(offset_value)))
	{
		key_count[6]++;
		if (key_count[6] == HIT_COUNT) {
			status->curr_key_num = DEVICE_NOTIFY_KEY_K12_TAP; 
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

static void adc_key2_process(KEY_MATRIX_STATUS_T *status)
{
	if (status == NULL)
		return;

    if (esp_efuse_vref_is_cali() == 2) 
	{
		status->adc_value = get_adc_voltage_tp(ADC_ATTEN_11db, status->channel_0);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,TWO Point adc:%d\r\n", status->channel_0, status->adc_value);
#endif
    } 
	else if (esp_efuse_vref_is_cali() == 1) 
	{
		status->adc_value = get_adc_voltage_filter(ADC_ATTEN_11db, status->channel_0);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,Vref adc:%d\r\n", status->channel_0, status->adc_value);
#endif
	} 
	else 
	{
		status->adc_value = adc1_get_voltage(status->channel_0);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,No calibration data, Raw:%d\r\n", status->channel_0, status->adc_value);
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
			offset_value = idel_value - KEY_MATRIX_ADC_KEY_2_IDEL_VOLTAGE;
			sum_of_idel  = 0;
			num = 0;
		}
		
	}
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

static void adc_vol_ctrl_process()
{
	int adc_value = 0;
	int vol_percent_value = 0;
	static int flag = 0;
	static int adc_value_sum = 0;
	static int volume_flag = 0;
//	int volume = 0;
	
	if (esp_efuse_vref_is_cali() == 2) 
	{
		adc_value = get_adc_voltage_tp(ADC_ATTEN_11db, ADC1_CHANNEL_5);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,TWO Point adc:%d\r\n", ADC1_CHANNEL_5, adc_value);
#endif
    } 
	else if (esp_efuse_vref_is_cali() == 1) 
	{
		adc_value = get_adc_voltage_filter(ADC_ATTEN_11db, ADC1_CHANNEL_5);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,Vref adc:%d\r\n", ADC1_CHANNEL_5, adc_value);
#endif
		flag++;
		adc_value_sum = adc_value_sum + adc_value;
//		printf("adc_value_sum 1 = %d\n", adc_value_sum);
		if (flag >= 5)
		{
//			printf("adc_value_sum 2 = %d\n", adc_value_sum);
			vol_percent_value = adc_value_sum/flag/3502.0 * 100;
//			printf("vol_percent_value 1 = %d\n", vol_percent_value);
			if (vol_percent_value > 100)
			{
				vol_percent_value = 100;
			}
			else if (vol_percent_value < 0)
			{
				vol_percent_value = 0;
			}
			vol_percent_value = 100 - vol_percent_value;
//			printf("vol_percent_value 2 = %d\n", vol_percent_value);
			if (vol_percent_value != volume_flag)
			{
				printf("\n\n****************\n\nin if !\n\n*************\n");
				MediaHalSetVolume(vol_percent_value);
				printf("MediaHalSetVolume volume = %d\n", vol_percent_value);
				volume_flag = vol_percent_value;
				flag = 0;
				adc_value_sum = 0;
			}
		flag = 0;
		adc_value_sum = 0;
		}
		
	} 
	else 
	{
		adc_value = adc1_get_voltage(ADC1_CHANNEL_5);
#ifdef PRINT_ADC_VALUE
        printf("channel %d,No calibration data, Raw:%d\r\n", ADC1_CHANNEL_5, adc_value);
#endif
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
			bat_cb(total_value*1.5/total_count/1000);
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
	adc_vol_ctrl_process(&key_matrix->vol_num);//ADC1_CHANNEL_5
	adc_battery_process(key_matrix->bat_cb);

	xTimerStart(key_matrix->tmr, KEY_SCAN_PERIOD_MS/portTICK_PERIOD_MS);

	return;
}

static void init_key1_status(KEY_MATRIX_STATUS_T *status, key_matrix_cb cb, adc1_channel_t channel_3)
{
	if (status == NULL)
	{
		return;
	}

	status->curr_key_num = DEVICE_NOTIFY_KEY_START;
	status->last_key_num = DEVICE_NOTIFY_KEY_START;
	status->cb = cb;
	status->channel_3 = channel_3;

	esp_adc1_cali_config(ADC_WIDTH_12Bit, ADC_ATTEN_11db, channel_3);
}

static void init_key2_status(KEY_MATRIX_STATUS_T *status, key_matrix_cb cb, adc1_channel_t channel_0)
{
	if (status == NULL)
	{
		return;
	}

	status->curr_key_num = DEVICE_NOTIFY_KEY_START;
	status->last_key_num = DEVICE_NOTIFY_KEY_START;
	status->cb = cb;
	status->channel_0 = channel_0;

	esp_adc1_cali_config(ADC_WIDTH_12Bit, ADC_ATTEN_11db, channel_0);
}

static void init_vol_status(KEY_MATRIX_STATUS_T *status, key_matrix_cb cb, adc1_channel_t channel_5)
{
	if (status == NULL)
	{
		return;
	}

	status->cb = cb;
	status->channel_5 = channel_5;

	esp_adc1_cali_config(ADC_WIDTH_12Bit, ADC_ATTEN_11db, channel_5);
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
	init_key1_status(&key_matrix->key1_status, _cb, ADC1_CHANNEL_3);
	init_key2_status(&key_matrix->key2_status, _cb, ADC1_CHANNEL_0);
	init_vol_status(&key_matrix->vol_num, _cb, ADC1_CHANNEL_5);
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


