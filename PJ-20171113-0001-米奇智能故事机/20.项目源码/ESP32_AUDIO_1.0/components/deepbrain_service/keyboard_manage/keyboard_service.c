#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#include "debug_log.h"
#include "driver/touch_pad.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "keyboard_manage.h"

#include "cJSON.h"
#include "deepbrain_service.h"
#include "touchpad.h"
#include "DeviceCommon.h"
#include "MediaControl.h"
#include "MediaHal.h"
#include "toneuri.h"
#include "baidu_api.h"
#include "http_api.h"
#include "deepbrain_api.h"
#include "device_api.h"
#include "socket.h"
#include "lwip/netdb.h"
#include "EspAudio.h"
#include "userconfig.h"
#include "Recorder.h"
#include "keyboard_manage.h"
#include "keyboard_service.h"
#include "nlp_service.h"
#include "wechat_service.h"
#include "chat_talk.h"
#include "auto_play_service.h"
#include "player_middleware.h"
#include "sd_music_manage.h"
#include "low_power_manage.h"
#include "mcu_serial_comm.h"
#include "flash_music_manage.h"
#include "led.h"
#include "ota_service.h"
#if MODULE_FREE_TALK
#include "free_talk.h"
#endif

#define LOG_TAG 						"keyboard service"
#define KEYBOARD_SERVICE_QUIT 			-1
#define VOLUME_STEP               		10
#define CHAT_OPEN						1
#define CHAT_CLOSE						0

typedef struct 
{
	MediaService *service;
	xQueueHandle msg_queue;
}KEYBOARD_SERVICE_HANDLE_T;

static KEYBOARD_SERVICE_HANDLE_T g_keyboard_service_handle = {0};

void key_matrix_callback(DEVICE_NOTIFY_KEY_T key_event)
{
	switch (key_event)
	{
		case DEVICE_NOTIFY_KEY_CHAT_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_CHAT_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_CHAT_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_CHAT_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_CHAT_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_CHAT_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_CHAT_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_CHAT_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_LED_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_LED_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_LED_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_LED_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_LED_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_LED_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_LED_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_LED_LONGPRESSED");
			break;
		}
		case DEVICE_NOTIFY_KEY_VOL_UP_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_VOL_UP_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_VOL_UP_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_VOL_UP_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_VOL_UP_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_VOL_UP_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_VOL_UP_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_VOL_UP_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_VOL_DOWN_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_VOL_DOWN_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_VOL_DOWN_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_VOL_DOWN_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_VOL_DOWN_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_VOL_DOWN_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_VOL_DOWN_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_VOL_DOWN_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_MENU_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_MENU_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_MENU_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_MENU_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_MENU_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_MENU_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_MENU_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_MENU_LONGPRESSED");
			break;
		}
	
		case DEVICE_NOTIFY_KEY_PREV_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_PREV_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_PREV_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_PREV_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_PREV_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_PREV_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_PREV_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_PREV_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_PLAY_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_PLAY_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_PLAY_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_PLAY_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_PLAY_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_PLAY_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_PLAY_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_PLAY_LONGPRESSED");
			break;
		}
	
		case DEVICE_NOTIFY_KEY_NEXT_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_NEXT_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_NEXT_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_NEXT_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_NEXT_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_NEXT_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_NEXT_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_NEXT_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_STORY_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_STORY_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_STORY_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_STORY_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_STORY_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_STORY_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_STORY_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_STORY_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_FW_UPDATE_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_FW_UPDATE_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_FW_UPDATE_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_FW_UPDATE_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_FW_UPDATE_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_FW_UPDATE_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_FW_UPDATE_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_FW_UPDATE_LONGPRESSED");
			break;
		}
				
		default:
			ESP_LOGE(LOG_TAG, "unknow key num %d", key_event);
			break;
	}

	if (g_keyboard_service_handle.msg_queue != NULL && get_ota_upgrade_state() != OTA_UPGRADE_STATE_UPGRADING)
	{
		xQueueSend(g_keyboard_service_handle.msg_queue, &key_event, 0);
	}
}

int keyborad_key_cancle(void)
{
	static uint32_t key_press_time = 0;
	uint32_t time_now = time(NULL);
	
	if (abs(key_press_time - time_now) < 2)
	{
		key_press_time = time_now;
		return 1;
	}
	else
	{
		key_press_time = time_now;
		return 0;
	}
}

static void task_key_event_process(void *pv)
{
    DEVICE_NOTIFY_KEY_T key_event = DEVICE_NOTIFY_KEY_START;
    KEYBOARD_SERVICE_HANDLE_T *handle = (KEYBOARD_SERVICE_HANDLE_T *) pv;
	MediaService *service = handle->service;
#if MODULE_MCU_SERIAL_COMM
	mcu_serial_comm_create(key_matrix_callback, set_battery_vol);
#endif
#if MODULE_ADC_KEYBOARD
	key_matrix_handle_t key_handle = adc_keyboard_matrix_init(key_matrix_callback, set_battery_vol);
#endif
    while (1) 
	{
		if (xQueueReceive(handle->msg_queue, &key_event, portMAX_DELAY) == pdPASS) 
		{			
			switch (key_event)
			{
				case DEVICE_NOTIFY_KEY_CHAT_PUSH:
				{//微聊
					if (service->_blocking == 0)
					{		
						if (keyborad_key_cancle())
						{
							break;
						}
						auto_play_pause();
						vTaskDelay(100);
						player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);
						vTaskDelay(500);
						wechat_record_start();
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE);
						vTaskDelay(3*1000);
					}
					break;
				}
				case DEVICE_NOTIFY_KEY_CHAT_RELEASE:
				{//微聊
					wechat_record_stop();
					break;
				}
				#if MODULE_FREE_TALK
				case DEVICE_NOTIFY_KEY_LED_TAP:
				{
					if (keyborad_key_cancle())
					{
						break;
					}
					
					if (service->_blocking == 0)
					{
						if (is_free_talk_running())
						{
							free_talk_stop();
							player_mdware_play_tone(FLASH_MUSIC_CLOSE_FREE_TALK);
						}
						else
						{
							auto_play_pause();
							vTaskDelay(100);
							set_sd_music_stop_state();
							free_talk_start();
						}						
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE);
						vTaskDelay(3*1000);
					}
					break;
				}
				#else
				case DEVICE_NOTIFY_KEY_LED_PUSH:
				{//聊天
					if (keyborad_key_cancle())
					{
						break;
					}
					
					if (service->_blocking == 0)
					{
						auto_play_pause();
						vTaskDelay(100);
						player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);
						vTaskDelay(500);
						chat_talk_start();
						set_sd_music_stop_state();
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE);
						vTaskDelay(3*1000);
					}
					break;
				}
				case DEVICE_NOTIFY_KEY_LED_RELEASE:
				{//聊天
					chat_talk_stop();
					break;
				}
				#endif
				case DEVICE_NOTIFY_KEY_VOL_DOWN_TAP:
				{//音量减
					ESP_LOGI(LOG_TAG, "vol down");
                    int vol;
					
                    if (MediaHalGetVolume(&vol) == 0) 
					{
                        if (MediaHalSetVolume(vol - VOLUME_STEP) == 0) 
						{
                            ESP_LOGI(LOG_TAG, "current vol = %d (line %d)",
                                     vol - VOLUME_STEP < 0 ? vol : vol - VOLUME_STEP, __LINE__);
                        }
                    }

					PlayerStatus player = {0};
	                service->getPlayerStatus((MediaService*)service, &player);
	                if (player.status != AUDIO_STATUS_PLAYING) 
					{
						player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);
	                } 
					break;
				}
				case DEVICE_NOTIFY_KEY_VOL_UP_TAP:
				{//音量加
					ESP_LOGI(LOG_TAG, "vol up");
                    int vol;
                    if (MediaHalGetVolume(&vol) == 0) 
					{
                        if (MediaHalSetVolume(vol + VOLUME_STEP) == 0)
						{
                            ESP_LOGI(LOG_TAG, "current vol = %d (line %d)",
                                     vol + VOLUME_STEP > 100 ? vol : vol + VOLUME_STEP, __LINE__);
                        }
                    }

					PlayerStatus player = {0};
	                service->getPlayerStatus((MediaService*)service, &player);
	                if (player.status != AUDIO_STATUS_PLAYING) 
					{
						player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);
	                } 
					break;
				}
				case DEVICE_NOTIFY_KEY_PREV_TAP:	
				{//上一曲
					if (keyborad_key_cancle())
					{
						break;
					}
					auto_play_pause();
					vTaskDelay(100);
					//EspAudioPause();
					player_mdware_play_tone(FLASH_MUSIC_PREV_SONG);
					vTaskDelay(2*1000);
					auto_play_prev();
					break;
				}
				case DEVICE_NOTIFY_KEY_NEXT_TAP:	
				{//下一曲
					if (keyborad_key_cancle())
					{
						break;
					}
					auto_play_pause();
					vTaskDelay(100);
					//EspAudioPause();
					player_mdware_play_tone(FLASH_MUSIC_NEXT_SONG);
					vTaskDelay(2*1000);
					auto_play_next();
					break;
				}
				case DEVICE_NOTIFY_KEY_MENU_TAP:	
				{//切换TF卡目录
					if (keyborad_key_cancle())
					{
						break;
					}
					auto_play_pause();
					sd_music_start();
					break;
				}
				case DEVICE_NOTIFY_KEY_PLAY_TAP:	
				{//暂停/播放
					if (keyborad_key_cancle())
					{
						break;
					}
					auto_play_pause_resume();
					break;
				}
				case DEVICE_NOTIFY_KEY_MENU_LONGPRESSED:	
				{//配网
					auto_play_pause();
					set_sd_music_stop_state();
					service->sendEvent((MediaService*) service, MEDIA_WIFI_SMART_CONFIG_REQUEST, NULL, 0);
					player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONFIG);
					break;
				}
				case DEVICE_NOTIFY_KEY_STORY_TAP:
				{//迪士尼故事
					if (keyborad_key_cancle())
					{
						break;
					}
					player_mdware_play_tone(FLASH_MUSIC_DISNEY_STORY);
					vTaskDelay(2500);
					flash_music_start();
					set_sd_music_stop_state();
					break;
				}
				case DEVICE_NOTIFY_KEY_STORY_LONGPRESSED:
				{//七彩灯
					switch_color_led();
					break;
				}
				default:
					break;
			}

			if (key_event == KEYBOARD_SERVICE_QUIT)
			{
				break;
			}
		}
    }

    vTaskDelete(NULL);
}


void keyboard_service_create(MediaService *self)
{
	g_keyboard_service_handle.service = self;
	g_keyboard_service_handle.msg_queue = xQueueCreate(5, sizeof(char *));
	
    if (xTaskCreate(task_key_event_process,
                    "task_key_event_process",
                    1024*3,
                    &g_keyboard_service_handle,
                    4,
                    NULL) != pdPASS) {

        ESP_LOGE(LOG_TAG, "ERROR creating task_key_event_process task! Out of memory?");
    }
}

void keyboard_service_delete(void)
{
	int key_event = KEYBOARD_SERVICE_QUIT;
	
	if (g_keyboard_service_handle.msg_queue != NULL)
	{
		xQueueSend(g_keyboard_service_handle.msg_queue, &key_event, 0);
	}
}

