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
#include "led_ctrl.h"
#include "wechat_service.h"
#include "free_talk.h"
#include "auto_play_service.h"
#include "player_middleware.h"
#include "sd_music_manage.h"
#include "low_power_manage.h"
#include "ota_service.h"
#include "volume_manage.h"

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

void key_matrix_callback(KEY_EVENT_T key_event)
{
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
    KEY_EVENT_T key_event = KEY_EVENT_IDEL;
    KEYBOARD_SERVICE_HANDLE_T *handle = (KEYBOARD_SERVICE_HANDLE_T *) pv;
	MediaService *service = handle->service;
	int chat_state = CHAT_CLOSE;
	
#if MODULE_MCU_SERIAL_COMM
	//mcu_serial_comm_create();
#endif

#if MODULE_ADC_KEYBOARD
	key_matrix_handle_t key_handle = adc_keyboard_matrix_init(key_matrix_callback, set_battery_vol);
#endif

    while (1) 
	{
		if (xQueueReceive(handle->msg_queue, &key_event, portMAX_DELAY) == pdPASS) 
		{
			update_device_sleep_time();
			ESP_LOGE(LOG_TAG, "check update_device_sleep_time");
			switch (key_event)
			{
				case KEY_EVENT_VOL_DOWN:
				{//音量减
					ESP_LOGE(LOG_TAG, "KEY_EVENT_VOL_DOWN");
                    int vol;
					int vol_flag = 0;
					
                    if (MediaHalGetVolume(&vol) == 0) 
					{
						ESP_LOGI(LOG_TAG, "before not_set vol = [%d] (line %d)", vol, __LINE__);
						if(vol >= (MIN_VOLUME + VOL_STEP_VALUE))
						{
							if (MediaHalSetVolume(vol - VOL_STEP_VALUE) == 0) 
							{
	                            ESP_LOGI(LOG_TAG, "current vol = %d (line %d)", vol - VOL_STEP_VALUE, __LINE__);
	                        }
						}
						else
						{
							if (MediaHalSetVolume(MIN_VOLUME) == 0)
							{
								ESP_LOGI(LOG_TAG, "current vol = %d (line %d)", MIN_VOLUME, __LINE__);
								vol_flag = 1;
							}
						}
                    }
					PlayerStatus player = {0};
	                service->getPlayerStatus((MediaService*)service, &player);
	                if (player.status != AUDIO_STATUS_PLAYING) 
					{
						if (vol_flag == 0)
						{
							player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);
							vTaskDelay(300);
						}
						else
						{
							player_mdware_play_tone(FLASH_MUSIC_DYY_VOLUME_ALREADY_LOWEST);
							vTaskDelay(1000);
						}
	                }
					break;
				}
				case KEY_EVENT_VOL_UP:
				{//音量加
					ESP_LOGE(LOG_TAG, "KEY_EVENT_VOL_UP");
                    int vol;
					int vol_flag = 0;
					
                    if (MediaHalGetVolume(&vol) == 0) 
					{
						ESP_LOGI(LOG_TAG, "before not_set vol = [%d] (line %d)", vol, __LINE__);
						if (vol <= (MAX_VOLUME - VOL_STEP_VALUE))
						{
							if (MediaHalSetVolume(vol + VOL_STEP_VALUE) == 0)
							{
		                        ESP_LOGI(LOG_TAG, "current vol = %d (line %d)", vol + VOL_STEP_VALUE, __LINE__);
		                    }
						}
						else
						{
							if (MediaHalSetVolume(MAX_VOLUME) == 0)
							{
								ESP_LOGI(LOG_TAG, "current vol = %d (line %d)", MIN_VOLUME, __LINE__);
								vol_flag = 1;
							}
						}
                    }
					
					PlayerStatus player = {0};
	                service->getPlayerStatus((MediaService*)service, &player);
	                if (player.status != AUDIO_STATUS_PLAYING) 
					{
						if (vol_flag == 0)
						{
							player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);
							vTaskDelay(300);
						}
						else
						{
							player_mdware_play_tone(FLASH_MUSIC_DYY_VOLUME_ALREADY_HIGHEST);
							vTaskDelay(1000);
						}
	                } 
					break;
				}
				case KEY_EVENT_PREV:	
				{//上一曲
					ESP_LOGE(LOG_TAG, "KEY_EVENT_PREV");
					if (keyborad_key_cancle())
					{
						break;
					}
					
					player_mdware_play_tone(FLASH_MUSIC_PREV_SONG);
					vTaskDelay(2*1000);
					auto_play_prev();
					break;
				}
				case KEY_EVENT_NEXT:	
				{//下一曲
					ESP_LOGE(LOG_TAG, "KEY_EVENT_NEXT");
					if (keyborad_key_cancle())
					{
						break;
					}
					
					player_mdware_play_tone(FLASH_MUSIC_NEXT_SONG);
					vTaskDelay(2*1000);
					auto_play_next();
					break;
				}
				case KEY_EVENT_CHAT_START:
				{//对话
					ESP_LOGE(LOG_TAG, "KEY_EVENT_CHAT_START");
					if (service->_blocking == 0)
					{						
						free_talk_stop();
						auto_play_stop();
						vTaskDelay(200);
						
						set_talk_tone_state(FREE_TALK_TONE_STATE_ENABLE_PLAY);
						//player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);
						free_talk_set_state(FREE_TALK_STATE_NORMAL);
						
						free_talk_start();
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE);
						vTaskDelay(3*1000);
					}
					break;
				}
				case KEY_EVENT_CH2EN:	
				{//中译英
					ESP_LOGE(LOG_TAG, "KEY_EVENT_CH2EN");
					if (service->_blocking == 0)
					{						
						free_talk_stop();
						auto_play_stop();
						vTaskDelay(200);

						player_mdware_play_tone(FLASH_MUSIC_CHINESE_ENGLISH_TRANSLATE);
						free_talk_set_state(FREE_TALK_STATE_TRANSLATE_CH_EN);
						vTaskDelay(1500);
	
						free_talk_start();
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE);
						vTaskDelay(3*1000);
					}
					break;
				}
				case KEY_EVENT_PUSH_PLAY:	
				{//暂停/播放
					ESP_LOGE(LOG_TAG, "KEY_EVENT_PUSH_PLAY");
					if (!keyborad_key_cancle())
					{
						free_talk_stop();
						auto_play_pause_resume();
					}
					break;
				}
				case KEY_EVENT_EN2CH:
				{//英译中
					ESP_LOGE(LOG_TAG, "KEY_EVENT_EN2CH");
					if (service->_blocking == 0)
					{	
						free_talk_stop();
						auto_play_stop();
						vTaskDelay(200);
						player_mdware_play_tone(FLASH_MUSIC_ENGLISH_CHINESE_TRANSLATE);
						free_talk_set_state(FREE_TALK_STATE_TRANSLATE_EN_CH);
						vTaskDelay(1500);
						free_talk_start();
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE);
						vTaskDelay(3*1000);
					}
					break;
				}
				case KEY_EVENT_WIFI:	
				{//配网
					ESP_LOGE(LOG_TAG, "KEY_EVENT_WIFI");
					if (is_free_talk_running())
					{
						free_talk_stop();
						auto_play_pause();
					}
					led_ctrl_set_mode(LED_CTRL_EYE_FLASHING_SLOW);
					service->sendEvent((MediaService*) service, MEDIA_WIFI_SMART_CONFIG_REQUEST, NULL, 0);
					player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_CONFIG);
					break;
				}
				case KEY_EVENT_WECHAT_START:
				{//微聊
					ESP_LOGE(LOG_TAG, "KEY_EVENT_WECHAT_START");
					if (keyborad_key_cancle())
					{
						break;
					}
					
					if (service->_blocking == 0)
					{
						chat_state = CHAT_OPEN;
						free_talk_stop();
						auto_play_pause();
						vTaskDelay(200);
						player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);
						vTaskDelay(300);
						wechat_record_start();
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE);
						vTaskDelay(3*1000);
					}
					break;
				}
				case KEY_EVENT_WECHAT_STOP:
				{//微聊
					ESP_LOGE(LOG_TAG, "KEY_EVENT_WECHAT_STOP");
					if (chat_state == CHAT_OPEN)
					{
						free_talk_stop();
						wechat_record_stop();
						chat_state = CHAT_CLOSE;
					}
					break;
				}
				case KEY_EVENT_SDCARD:
				{
					//sd卡目录切换
					ESP_LOGE(LOG_TAG, "KEY_EVENT_SDCARD");
					if (keyborad_key_cancle())
					{
						break;
					}

					free_talk_stop();
					auto_play_stop();
					vTaskDelay(200);
					sd_music_start();
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

	adc_keyboard_matrix_free(key_handle);
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

