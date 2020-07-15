#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "device_api.h"
#include "esp_log.h"
#include "EspAudio.h"
#include "toneuri.h"
#include "led_ctrl.h"
#include "device_api.h"
#include "debug_log.h"
#include "deep_sleep_manage.h"
#include "userconfig.h"
#include "led_ctrl.h"
#include "player_middleware.h"
#include "low_power_manage.h"
#include "semaphore_lock.h"
#include "WifiManager.h"
#include "mcu_serial_comm.h"

#define LOW_POWER_VOLTAGE		3.51				//播报低电量提示电压区间最高值
#define OFF_POWER_VOLTAGE		3.50				//播报关机提示电压
#define DEVICE_SLEEP_TIME		(5*60)				//5分钟没有动作，则机器自动进入休眠
#define TAG_POWER_MANAGE 		"POWER_MANAGE"

//电源管理
typedef struct
{
	xQueueHandle msg_queue;	 //消息队列
	uint32_t sleep_timestamp;//睡眠时间
}POWER_MANAGE_HANDLE_T;

static SemaphoreHandle_t g_lock_power_manage = NULL;
static POWER_MANAGE_HANDLE_T *g_power_manage_handle = NULL;

static int judge_value(int value)
{
	static int init_value = -1;
	
	if (init_value == value)
	{
		return 0;
	}
	else
	{
		init_value = value;
		return 1;
	}
}

static void task_power_manage(void* pv)
{
	float battery_vol = 0.0;
	uint32_t play_time = 0;
	uint32_t play_low_first = 0;
	int power_supply_state = 0;
	POWER_MANAGE_HANDLE_T * power_handle = (POWER_MANAGE_HANDLE_T *)pv;
	
	vTaskDelay(10*1000);
	while (1)
	{
        if (xQueueReceive(power_handle->msg_queue, &battery_vol, portMAX_DELAY) == pdPASS)
	    {
	       	ESP_LOGI(TAG_POWER_MANAGE, "battery vol = %.2f", battery_vol);
			
			connect_wifi_error_manage();	//联网出错处理
			
			//检测是否充电
			power_supply_state = get_battery_charging_state();
			//printf("********* power_supply_state = [%d]\n", power_supply_state);
			if (judge_value(power_supply_state))
			{
				if (power_supply_state == 1)
				{
					uart_esp32_status_mode(UART_ESP32_STATUS_MODE_POWER_SUPPLY_STATU);
				}
				else
				{
					uart_esp32_status_mode(UART_ESP32_STATUS_MODE_NOT_POWER_SUPPLY_STATU);
				}
			}
#if 0			
			//设备自动休眠
			if (abs(get_device_sleep_time() - time(NULL)) >= DEVICE_SLEEP_TIME)
			{
				player_mdware_play_tone(FLASH_MUSIC_DEVICE_SLEEP);
				vTaskDelay(4*1000);
				//led_ctrl_set_mode(LED_CTRL_ALL_CLOSE);
				vTaskDelay(500);
				deep_sleep_manage();
			}

			//检测是否充电
			if (get_battery_charging_state())
			{
				ESP_LOGI(TAG_POWER_MANAGE, "usb charging now");
				vTaskDelay(1000);
				continue;
			}

			//低电量休眠
			if(battery_vol <= LOW_POWER_VOLTAGE)
			{
				//led_ctrl_set_mode(LED_CTRL_EYE_FLASHING_FAST);
				if (battery_vol <= OFF_POWER_VOLTAGE && play_low_first == 1)
				{
					EspAudioStop();
					
					player_mdware_play_tone(FLASH_MUSIC_POWER_OFF);
					vTaskDelay(4*1000);
					//led_ctrl_set_mode(LED_CTRL_ALL_CLOSE);
					vTaskDelay(500);
					deep_sleep_manage();
				}
				else
				{
					if (play_time == 0 
						 || abs(play_time - time(NULL)) >= 30)
					{
						player_mdware_play_tone(FLASH_MUSIC_BATTERY_LOW);
						vTaskDelay(3*1000);
						play_time = time(NULL);
						play_low_first = 1;
					}
				}
			}
#endif
		}
	}
	vTaskDelete(NULL);
}

void low_power_manage_create()
{
	g_power_manage_handle = (POWER_MANAGE_HANDLE_T *)esp32_malloc(sizeof(POWER_MANAGE_HANDLE_T));
	if (g_power_manage_handle == NULL)
	{
		return;
	}
	memset(g_power_manage_handle, 0, sizeof(POWER_MANAGE_HANDLE_T));

	g_power_manage_handle->sleep_timestamp = time(NULL);
	SEMPHR_CREATE_LOCK(g_lock_power_manage);
	g_power_manage_handle->msg_queue = xQueueCreate(5, sizeof(float));

	if (xTaskCreate(
		task_power_manage, 
		"task_power_manage", 
		512*4, 
		g_power_manage_handle, 
		10, 
		NULL) != pdPASS) 
    {
        ESP_LOGE(TAG_POWER_MANAGE, "Error create task_power_manage failed");
		vQueueDelete(g_power_manage_handle->msg_queue);
		esp32_free(g_power_manage_handle);
		g_power_manage_handle = NULL;
		SEMPHR_DELETE_LOCK(g_lock_power_manage);
		g_lock_power_manage = NULL;
    }
}

int get_battery_charging_state(void)
{
	int led_level = gpio_get_level(CHARGING_STATE_GPIO);
	ESP_LOGI(TAG_POWER_MANAGE, "check_charging_state level=%d", led_level);
	
	return led_level;
}

void set_battery_vol(const float vol)
{
	if (g_power_manage_handle != NULL && g_power_manage_handle->msg_queue != NULL)
	{
		xQueueSend(g_power_manage_handle->msg_queue, &vol, 0);
	}
}

void update_device_sleep_time(void)
{
	SEMPHR_TRY_LOCK(g_lock_power_manage);
	if (g_power_manage_handle != NULL)
	{
		g_power_manage_handle->sleep_timestamp = time(NULL);
	}
	SEMPHR_TRY_UNLOCK(g_lock_power_manage);	
}

uint32_t get_device_sleep_time(void)
{
	uint32_t sleep_time = time(NULL);
	
	SEMPHR_TRY_LOCK(g_lock_power_manage);
	if (g_power_manage_handle != NULL)
	{
		sleep_time = g_power_manage_handle->sleep_timestamp;
	}
	SEMPHR_TRY_UNLOCK(g_lock_power_manage);	

	return sleep_time;
}

