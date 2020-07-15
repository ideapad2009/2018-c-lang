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
//#include "chat_talk.h"
#include "auto_play_service.h"
#include "player_middleware.h"
#include "sd_music_manage.h"
#include "low_power_manage.h"
#include "mcu_serial_comm.h"
#include "flash_music_manage.h"
#include "led.h"
#include "ota_service.h"
#include "volume_manage.h"

#if MODULE_FREE_TALK
#include "free_talk.h"
#endif

#define LOG_TAG 						"keyboard service"
#define KEYBOARD_SERVICE_QUIT 			-1
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
		case DEVICE_NOTIFY_KEY_K1_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K1_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K1_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K1_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K1_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K1_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K1_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K1_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_K2_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K2_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K2_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K2_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K2_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K2_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K2_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K2_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_K3_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K3_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K3_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K3_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K3_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K3_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K3_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K3_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_K4_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K4_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K4_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K4_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K4_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K4_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K4_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K4_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_K5_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K5_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K5_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K5_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K5_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K5_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K5_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K5_LONGPRESSED");
			break;
		}
	
		case DEVICE_NOTIFY_KEY_K6_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K6_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K6_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K6_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K6_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K6_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K6_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K6_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_K7_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K7_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K7_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K7_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K7_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K7_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K7_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K7_LONGPRESSED");
			break;
		}
	
		case DEVICE_NOTIFY_KEY_K8_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K8_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K8_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K8_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K8_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K8_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K8_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K8_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_K9_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K9_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K9_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K9_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K9_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K9_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K9_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K9_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_K10_TAP:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K10_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K10_PUSH:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K10_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K10_RELEASE:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K10_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K10_LONGPRESSED:
		{
			ESP_LOGE(LOG_TAG, "DEVICE_NOTIFY_KEY_K10_LONGPRESSED");
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
				case DEVICE_NOTIFY_KEY_K1_PUSH:
				{//微聊
					sd_music_manage_stop_first_play();
					if (service->_blocking == 0)
					{		
						if (keyborad_key_cancle())
						{
							break;
						}
						auto_play_pause();
						vTaskDelay(100);
						if (is_free_talk_running())
						{
							free_talk_stop();//关闭录音
						}
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
				case DEVICE_NOTIFY_KEY_K1_RELEASE:
				{//微聊
					wechat_record_stop();
					break;
				}
				#if MODULE_FREE_TALK
				case DEVICE_NOTIFY_KEY_K2_TAP:
				{
					sd_music_manage_stop_first_play();
					if (keyborad_key_cancle())
					{
						break;
					}
					
					if (service->_blocking == 0)
					{
						auto_play_pause();
						vTaskDelay(100);
						set_sd_music_stop_state();
						set_talk_tone_state(FREE_TALK_TONE_STATE_ENABLE_PLAY);
						free_talk_start();
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE);
						vTaskDelay(3*1000);
					}
					break;
				}
				#else
				case DEVICE_NOTIFY_KEY_K2_PUSH:
				{//聊天
					sd_music_manage_stop_first_play();
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
				case DEVICE_NOTIFY_KEY_K2_RELEASE:
				{//聊天
					chat_talk_stop();
					break;
				}
				#endif
				case DEVICE_NOTIFY_KEY_K9_TAP:
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
				case DEVICE_NOTIFY_KEY_K7_TAP:
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
								ESP_LOGI(LOG_TAG, "current vol = %d (line %d)", MAX_VOLUME, __LINE__);
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
				case DEVICE_NOTIFY_KEY_K3_TAP:	
				{//上一曲
					if (keyborad_key_cancle())
					{
						break;
					}
					if (is_free_talk_running())
					{
						free_talk_stop();//关闭录音
					}
					auto_play_pause();
					vTaskDelay(100);
					//EspAudioPause();
					player_mdware_play_tone(FLASH_MUSIC_PREV_SONG);
					vTaskDelay(2*1000);
					auto_play_prev();
					break;
				}
				case DEVICE_NOTIFY_KEY_K4_TAP:	
				{//下一曲
					if (keyborad_key_cancle())
					{
						break;
					}
					if (is_free_talk_running())
					{
						free_talk_stop();//关闭录音
					}
					auto_play_pause();
					vTaskDelay(100);
					//EspAudioPause();
					player_mdware_play_tone(FLASH_MUSIC_NEXT_SONG);
					vTaskDelay(2*1000);
					auto_play_next();
					break;
				}
				case DEVICE_NOTIFY_KEY_K8_TAP:	
				{//切换TF卡目录
					sd_music_manage_stop_first_play();
					if (keyborad_key_cancle())
					{
						break;
					}

					auto_play_stop();//停止自动播放
					if (is_free_talk_running())
					{
						free_talk_stop();//关闭录音
					}
					sd_music_start();
					break;
				}
				case DEVICE_NOTIFY_KEY_K5_TAP:	
				{//暂停/播放
					if (keyborad_key_cancle())
					{
						break;
					}
					if (is_free_talk_running())
					{
						free_talk_stop();//关闭录音
					}
					auto_play_pause_resume();
					break;
				}
				case DEVICE_NOTIFY_KEY_K8_LONGPRESSED:	
				{//配网
					sd_music_manage_stop_first_play();
					auto_play_stop();//停止自动播放
					vTaskDelay(200);
					if (is_free_talk_running())
					{
						free_talk_stop();//关闭录音
					}
					auto_play_pause();
					set_sd_music_stop_state();
					service->sendEvent((MediaService*) service, MEDIA_WIFI_SMART_CONFIG_REQUEST, NULL, 0);
					player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONFIG);
					break;
				}
				case DEVICE_NOTIFY_KEY_K10_TAP:
				{//夜灯开关
					night_light_switch();
					/*迪士尼故事
					sd_music_manage_stop_first_play();
					if (keyborad_key_cancle())
					{
						break;
					}
					
					auto_play_stop();//停止自动播放
					vTaskDelay(200);
					if (is_free_talk_running())
					{
						free_talk_stop();//关闭录音
					}
					player_mdware_play_tone(FLASH_MUSIC_DISNEY_STORY);
					vTaskDelay(2500);
					flash_music_start();
					set_sd_music_stop_state();
					*/
					break;
				}
				case DEVICE_NOTIFY_KEY_K6_TAP:
				{//七彩灯开关
					color_led_switch();
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

