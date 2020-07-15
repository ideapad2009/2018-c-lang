#include "auto_play_service.h"
#include "debug_log.h"
#include "deep_sleep_manage.h"
#include "device_api.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "EspAudio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "free_talk.h"
#include "led.h"
#include "player_middleware.h"
#include "toneuri.h"
#include "userconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define KEY_MATRIX_DIVISON 		(float)(3.3/4095)
#define LOW_POWER_VOLTAGE		3.61				//播报低电量提示电压区间最高值
//#define OFF_POWER_VOLTAGE		3.60				//播报关机提示电压
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
				int flag_num = 0;
				
				auto_play_pause();	//关闭正在播放的音频
				vTaskDelay(100);	//为以上操作延时，以保证能正常关闭
				if (is_free_talk_running())		//检测自由对话是否开启
				{
					free_talk_stop();//关闭录音
				}
				
				do
				{
					player_mdware_play_tone(FLASH_MUSIC_POWER_OFF);
					flag_num++;
					if (flag_num < 5)
					{
						vTaskDelay(12*1000);
					}
					else	//播第五遍关机提示音后自动关机
					{
						vTaskDelay(3*1000);
						if (check_charging_state() != 1)	//第五遍关机提示音播完之前接入电源不会关机
						{
							color_led_off();
							night_light_off();
							deep_sleep_manage();
						}
					}
				}while(check_charging_state() != 1);
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
