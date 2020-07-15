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
#include "mcu_serial_comm.h"
#include "translate_manage.h"
#include "volume_manage.h"
#include "WifiManager.h"
#include "functional_running_state_check.h"
#include "prevent_child_trigger_manage.h"

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
	static int led_flag = 1;
	int sleep_flag = 0;
	static int wifi_flag = 0;
	
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
//			update_device_sleep_time();
//			ESP_LOGE(LOG_TAG, "check update_device_sleep_time");
			FUNCTIONAL_TYIGGER_KEY_E key_state = get_functional_tyigger_key_state();
			switch (key_event)
			{
				case KEY_EVENT_VOL_DOWN:
				{//音量减 短按K5
					ESP_LOGE(LOG_TAG, "KEY_EVENT_VOL_DOWN");

					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
						vTaskDelay(1000);
						break;
					}
					
					sleep_flag = 1;
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
				{//音量加 短按K3
					ESP_LOGE(LOG_TAG, "KEY_EVENT_VOL_UP");

					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
						vTaskDelay(1000);
						break;
					}
					
					sleep_flag = 1;
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
				{//上一曲 长按K5
					ESP_LOGE(LOG_TAG, "KEY_EVENT_PREV");

					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
						vTaskDelay(1000);
						break;
					}
					
					sleep_flag = 1;
					if (keyborad_key_cancle())
					{
						break;
					}
					free_talk_stop();
					player_mdware_play_tone(FLASH_MUSIC_PREV_SONG);
					vTaskDelay(2*1000);
					auto_play_prev();
					break;
				}
				case KEY_EVENT_NEXT:	
				{//下一曲 长按K3
					ESP_LOGE(LOG_TAG, "KEY_EVENT_NEXT");

					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
						vTaskDelay(1000);
						break;
					}
					
					sleep_flag = 1;
					if (keyborad_key_cancle())
					{
						break;
					}
					free_talk_stop();
					player_mdware_play_tone(FLASH_MUSIC_NEXT_SONG);
					vTaskDelay(2*1000);
					auto_play_next();
					break;
				}
				case KEY_EVENT_CHAT_START:
				{//语聊 短按K2 
					ESP_LOGE(LOG_TAG, "KEY_EVENT_CHAT_START");

					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
						vTaskDelay(1000);
						break;
					}
					
					sleep_flag = 1;
					if (service->_blocking == 0)
					{						
						free_talk_stop();
						auto_play_stop();
						vTaskDelay(200);
						set_talk_tone_state(FREE_TALK_TONE_STATE_DISABLE_PLAY);
						free_talk_tone();//使启动后每次的提示音不同
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
				case KEY_EVENT_TRANSLATE:
				{//中英互译 短按K6
					ESP_LOGE(LOG_TAG, "KEY_EVENT_TRANSLATE");

					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
						vTaskDelay(1000);
						break;
					}
					
					sleep_flag = 1;
					if (service->_blocking == 0)
					{						
						free_talk_stop();
						auto_play_stop();
						vTaskDelay(200);
						
						translate_tone();//使每次提示音不同
						free_talk_set_state(FREE_TALK_STATE_TRANSLATE_EN_OR_CH);
	
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
				{//暂停/播放 短按K4
					ESP_LOGE(LOG_TAG, "KEY_EVENT_PUSH_PLAY");

					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
						vTaskDelay(1000);
						break;
					}
					
					sleep_flag = 1;
					if (!keyborad_key_cancle())
					{
						free_talk_stop();
						auto_play_pause_resume();
					}
					break;
				}
				case KEY_EVENT_WIFI:	
				{//配网 长按K4
					ESP_LOGE(LOG_TAG, "KEY_EVENT_WIFI");					

					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
						vTaskDelay(1000);
						break;
					}
					
					sleep_flag = 1;
					if (get_wifi_mode_state() == WIFI_MODE_OFF)//判断是否已关闭配网
					{
						free_talk_stop();
						auto_play_stop();
						vTaskDelay(200);
						wifi_smart_connect_start();
						vTaskDelay(500);
						player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_CONFIG);
					}
					else
					{
						wifi_off_start();
						player_mdware_play_tone(FLASH_MUSIC_EXIT_NETWORK_MODE);
					}
					break;
				}
				case KEY_EVENT_WECHAT_START:
				{//微聊 按住K1
					ESP_LOGE(LOG_TAG, "KEY_EVENT_WECHAT_START");

					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
						vTaskDelay(1000);
						break;
					}
					
					sleep_flag = 1;
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

					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						break;
					}
					
					sleep_flag = 1;
					if (chat_state == CHAT_OPEN)
					{
						free_talk_stop();
						wechat_record_stop();
						chat_state = CHAT_CLOSE;
					}
					break;
				}
				case KEY_EVENT_SDCARD:
				{//sd卡目录切换 K6
					ESP_LOGE(LOG_TAG, "KEY_EVENT_SDCARD");

					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
						vTaskDelay(1000);
						break;
					}
					
					sleep_flag = 1;
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
				case KEY_EVENT_HEAD_LED_SWITCH:
				{//头灯 耳灯 长按K6
					ESP_LOGE(LOG_TAG, "KEY_EVENT_HEAD_LED_SWITCH");

					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
						vTaskDelay(1000);
						break;
					}
					
					sleep_flag = 1;
					printf("led_flag = [%d]\n", led_flag);
					if (led_flag == 0)
					{
						led_ctrl_set_mode(LED_CTRL_EAR_OPEN);
						led_ctrl_set_mode(LED_CTRL_HEAD_OPEN);
						led_flag = 1;
					}
					else
					{
						led_ctrl_set_mode(LED_CTRL_EAR_CLOSE);
						led_ctrl_set_mode(LED_CTRL_HEAD_CLOSE);
						led_flag = 0;
					}
					break;
				}
				case KEY_EVENT_CHILD_LOCK:
				{
					ESP_LOGE(LOG_TAG, "KEY_EVENT_CHILD_LOCK");
					
					prevent_child_trigger();
					break;
				}
				case KEY_EVENT_OTHER_KEY_CHILD_LOCK_PLAY:
				{
					ESP_LOGE(LOG_TAG, "KEY_EVENT_OTHER_KEY_CHILD_LOCK_PLAY");
					if (key_state == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
						vTaskDelay(1000);
						break;
					}
					break;
				}
				default:
					break;
			}

			if (key_event == KEYBOARD_SERVICE_QUIT)
			{
				break;
			}

			if (sleep_flag == 1)
			{
				update_device_sleep_time();
				ESP_LOGE(LOG_TAG, "check update_device_sleep_time");
				sleep_flag = 0;
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
                    512*6,
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

