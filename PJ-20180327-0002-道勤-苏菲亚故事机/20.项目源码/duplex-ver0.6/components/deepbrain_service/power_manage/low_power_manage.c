#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "device_api.h"
#include "debug_log.h"
#include "deep_sleep_manage.h"
#include "userconfig.h"
#include <time.h>
#include "player_middleware.h"

#define KEY_MATRIX_DIVISON 		(float)(3.3/4095)
#define LOW_POWER_VOLTAGE		3.61				//播报低电量提示电压区间最高值
#define OFF_POWER_VOLTAGE		3.60				//播报关机提示电压
#define MIN_INTERVAL_TIME		(20*1000)			//20秒
#define CHARGING_STATE_OFF		0
//#define CHARGING_STATE_GPIO		0				//板子无IO，暂时使用0		
#define TAG_POWER_MANAGE 		"POWER_MANAGE"

static xQueueHandle xQueueLowPowerManage;

int check_charging_state(void);

void set_battery_vol(float vol)
{
	if (xQueueLowPowerManage)
	{
		xQueueSend(xQueueLowPowerManage, &vol, 0);
	}
}

static void _low_power_manage(void* pv)
{
	float battery_vol = 0.0;
	uint32_t play_time = 0;
	uint32_t play_low_first = 0;
	
	vTaskDelay(10*1000);
	while (1)
	{
        if (xQueueReceive(xQueueLowPowerManage, &battery_vol, portMAX_DELAY) == pdPASS)
        {
        	ESP_LOGI(TAG_POWER_MANAGE, "battery vol = %.2f", battery_vol);
			
		if (check_charging_state())
			{
				ESP_LOGI(TAG_POWER_MANAGE, "usb charging now");
				vTaskDelay(1000);
				continue;
			}
		
			if(battery_vol <= LOW_POWER_VOLTAGE)
			{
				if (battery_vol <= OFF_POWER_VOLTAGE && play_low_first == 1)
				{
					EspAudioStop();
					
					player_mdware_play_tone(FLASH_MUSIC_POWER_OFF);
					vTaskDelay(4*1000);
					deep_sleep_manage();
				}
				else
				{
					if (play_time == 0 
						 || abs(play_time - time(NULL)) >= 30)
					{
						player_mdware_play_tone(FLASH_MUSIC_BATTERY_POWER_LOW);
						vTaskDelay(3*1000);
						play_time = time(NULL);
						play_low_first = 1;
					}
				}
			}
		}
		
	}
	vTaskDelete(NULL);
}

void low_power_manage_init()
{
	xQueueLowPowerManage = xQueueCreate(5, sizeof(float));
	xTaskCreate(_low_power_manage, "_low_power_manage", 1024*2, NULL, 10, NULL);
}

void low_power_manage_set(float vol)
{
	if (xQueueLowPowerManage)
	{
		xQueueSend(xQueueLowPowerManage, &vol, 0);
	}
}

int check_charging_state(void)
{
	int led_level = gpio_get_level(CHARGING_STATE_GPIO);
	ESP_LOGI(TAG_POWER_MANAGE, "check_charging_state level=%d", led_level);
	return led_level;
}
