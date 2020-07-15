#include "auto_play_service.h"
#include "driver/adc.h"
#include "device_api.h"
#include "driver/gpio.h"
#include "debug_log.h"
#include "deep_sleep_manage.h"
#include "EspAudio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "free_talk.h"
#include "led_ctrl.h"
#include "low_power_manage.h"
#include "mcu_serial_comm.h"
#include "player_middleware.h"
#include "semaphore_lock.h"
#include "toneuri.h"
#include "userconfig.h"
#include "WifiManager.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOW_POWER_VOLTAGE		3.61				//播报低电量提示电压区间最高值
//#define OFF_POWER_VOLTAGE		3.60				//播报关机提示电压
#define DEVICE_SLEEP_TIME		(20*60)				//20分钟没有动作，则机器自动进入休眠
#define TAG_POWER_MANAGE 		"POWER_MANAGE"

//电源管理
typedef struct
{
	xQueueHandle msg_queue;	 //消息队列
	uint32_t sleep_timestamp;//睡眠时间
}POWER_MANAGE_HANDLE_T;

static SemaphoreHandle_t g_lock_power_manage = NULL;
static POWER_MANAGE_HANDLE_T *g_power_manage_handle = NULL;

static void task_power_manage(void* pv)
{
	float battery_vol = 0.0;
	uint32_t play_time = 0;
	uint32_t play_low_first = 0;
	POWER_MANAGE_HANDLE_T * power_handle = (POWER_MANAGE_HANDLE_T *)pv;
	
	vTaskDelay(10*1000);
	while (1)
	{
        if (xQueueReceive(power_handle->msg_queue, &battery_vol, portMAX_DELAY) == pdPASS)
	    {
	       	ESP_LOGI(TAG_POWER_MANAGE, "battery vol = %.2f", battery_vol);
			
			connect_wifi_error_manage();	//联网出错处理
			
			//设备自动休眠
			if (abs(get_device_sleep_time() - time(NULL)) >= DEVICE_SLEEP_TIME)
			{
				player_mdware_play_tone(FLASH_MUSIC_DEVICE_SLEEP);
				vTaskDelay(4*1000);
				led_ctrl_set_mode(LED_CTRL_ALL_CLOSE);
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
				int flag_num = 1;
				
				auto_play_pause();	//关闭正在播放的音频
				vTaskDelay(100);	//为以上操作延时，以保证能正常关闭
				if (is_free_talk_running())		//检测自由对话是否开启
				{
					free_talk_stop();//关闭录音
				}
				
				do
				{
					if (flag_num < 5)
					{
						player_mdware_play_tone(FLASH_MUSIC_BATTERY_LOW);
						vTaskDelay(14*1000);
					}
					else	//播第五遍关机提示音后自动关机
					{
						player_mdware_play_tone(FLASH_MUSIC_POWER_OFF);
						vTaskDelay(4*1000);
						if (get_battery_charging_state() != 1)	//第五遍关机提示音播完之前接入电源不会关机
						{
							led_ctrl_set_mode(LED_CTRL_ALL_CLOSE);
							vTaskDelay(500);
							deep_sleep_manage();
						}
					}
					flag_num++;
				}while(get_battery_charging_state() != 1);
			}
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

