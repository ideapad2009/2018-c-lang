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
//#include "chat_talk.h"
#include "free_talk.h"
#include "auto_play_service.h"
#include "player_middleware.h"
#include "sd_music_manage.h"
#include "low_power_manage.h"
#include "mcu_serial_comm.h"
#include "functional_running_state_check.h"
#include "collection_function.h"
#include "sleep_type_switch.h"
#include "flash_music_manage.h"
#include "led.h"
#include "ota_service.h"
#include "prevent_child_trigger_manage.h"
#include "guo_xue_function_manage.h"
#include "bedtime_story_key_manage.h"
#include "SDCardConfig.h"

#define KEYBOARD_TAG 						"keyboard service"
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
	int key_event_flag = 0;
	static int key_num = 100;
	
	switch (key_event)
	{
		case DEVICE_NOTIFY_KEY_K1_TAP:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K1_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K1_PUSH:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K1_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K1_RELEASE:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K1_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K1_LONGPRESSED:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K1_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_K2_TAP:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K2_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K2_PUSH:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K2_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K2_RELEASE:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K2_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K2_LONGPRESSED:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K2_LONGPRESSED");
			break;
		}
	
		case DEVICE_NOTIFY_KEY_K3_TAP:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K3_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K3_PUSH:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K3_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K3_RELEASE:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K3_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K3_LONGPRESSED:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K3_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_K4_TAP:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K4_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K4_PUSH:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K4_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K4_RELEASE:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K4_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K4_LONGPRESSED:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K4_LONGPRESSED");
			break;
		}

		case DEVICE_NOTIFY_KEY_K5_TAP:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K5_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K5_PUSH:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K5_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K5_RELEASE:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K5_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K5_LONGPRESSED:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K5_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_K6_TAP:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K6_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K6_PUSH:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K6_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K6_RELEASE:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K6_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K6_LONGPRESSED:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K6_LONGPRESSED");
			break;
		}
	
		case DEVICE_NOTIFY_KEY_K7_TAP:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K7_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K7_PUSH:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K7_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K7_RELEASE:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K7_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K7_LONGPRESSED:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K7_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_K8_TAP:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K8_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K8_PUSH:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K8_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K8_RELEASE:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K8_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K8_LONGPRESSED:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K8_LONGPRESSED");
			break;
		}

		case DEVICE_NOTIFY_KEY_K9_TAP:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K9_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K9_PUSH:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K9_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K9_RELEASE:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K9_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K9_LONGPRESSED:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K9_LONGPRESSED");
			break;
		}
		
		case DEVICE_NOTIFY_KEY_K10_TAP:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K10_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K10_PUSH:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K10_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K10_RELEASE:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K10_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K10_LONGPRESSED:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K10_LONGPRESSED");
			break;
		}
	
		case DEVICE_NOTIFY_KEY_K11_TAP:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K11_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K11_PUSH:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K11_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K11_RELEASE:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K11_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K11_LONGPRESSED:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K11_LONGPRESSED");
			break;
		}
		case DEVICE_NOTIFY_KEY_K12_TAP:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K12_TAP");
			break;
		}
		case DEVICE_NOTIFY_KEY_K12_PUSH:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K12_PUSH");
			break;
		}
		case DEVICE_NOTIFY_KEY_K12_RELEASE:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K12_RELEASE");
			break;
		}
		case DEVICE_NOTIFY_KEY_K12_LONGPRESSED:
		{
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K12_LONGPRESSED");
			break;
		}
				
		default:
			ESP_LOGE(KEYBOARD_TAG, "unknow key num %d", key_event);
			break;
	}

	if (key_event == DEVICE_NOTIFY_KEY_K1_LONGPRESSED || key_event == DEVICE_NOTIFY_KEY_K8_LONGPRESSED)
	{
		if (key_event == DEVICE_NOTIFY_KEY_K1_LONGPRESSED)
		{
			key_event_flag = 1;
			key_num = key_event;
		}
		else
		{
			key_event_flag = 2;
		}
	}
	
	if (key_event_flag != 0)
	{
		if (key_event != key_num)
		{
			key_event = DEVICE_NOTIFY_KEY_K11_LONGPRESSED;
			key_num = 100;
			ESP_LOGE(KEYBOARD_TAG, "DEVICE_NOTIFY_KEY_K11_LONGPRESSED");
		}
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
#if MODULE_ADC_KEYBOARD
	key_matrix_handle_t key_handle = adc_keyboard_matrix_init(key_matrix_callback, set_battery_vol);
#endif
    while (1) 
	{
		if (xQueueReceive(handle->msg_queue, &key_event, portMAX_DELAY) == pdPASS) 
		{			
			switch (key_event)
			{
				case DEVICE_NOTIFY_KEY_K1_TAP://上一首
				{
					if (keyborad_key_cancle())
					{
						break;
					}
					player_mdware_play_tone(FLASH_MUSIC_PREV_SONG);
					vTaskDelay(2*1000);
					auto_play_prev();
					break;
				}
				case DEVICE_NOTIFY_KEY_K2_TAP://对话
				{
					if (get_functional_tyigger_key_state() == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_B_27_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
					}
					else
					{
						if (get_wifi_state() != WIFI_ON_STATE)
						{
							player_mdware_play_tone(FLASH_MUSIC_A_05_PROMPT_FIRST_CONNECT_THE_NETWORK);
						}
						else
						{
							if (get_free_talk_flag() == FREE_TALK_STATE_FLAG_START_STATE)
							{
								//auto_play_pause();
								auto_play_stop();
								color_led_flicker_stop();
								color_led_off();
								set_free_talk_pause();//关闭自动对话
								free_talk_set_mode(FREE_TALK_MODE_STOP);
								set_free_talk_flag(FREE_TALK_STATE_FLAG_STOP_STATE);
								free_talk_stop();//关闭录音
								set_functional_tyigger_key_state(FUNCTIONAL_TYIGGER_KEY_INITIAL_STATE);
								player_mdware_play_tone(FLASH_MUSIC_B_28_SHUT_DOWN_CONTINUOUS_CHAT);
								vTaskDelay(3000);
							}
							else
							{
								//auto_play_pause();
								auto_play_stop();
								set_free_talk_flag(FREE_TALK_STATE_FLAG_START_STATE);
								free_talk_start();//启动录音
								free_talk_set_mode(FREE_TALK_MODE_VAD_WAKEUP);//设为静音状态检测模式
								player_mdware_play_tone(FLASH_MUSIC_E_04_CHAT_SEND_TONE);
								vTaskDelay(1000);
								set_functional_tyigger_key_state(FUNCTIONAL_TYIGGER_KEY_SPEECH_RECOGNITION_STATE);
							}
						}
					}
					break;
				}
				case DEVICE_NOTIFY_KEY_K3_TAP://国学
				{
					if (get_functional_tyigger_key_state() == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_B_27_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
					}
					else
					{
						if (get_wifi_state() != WIFI_ON_STATE)
						{
							player_mdware_play_tone(FLASH_MUSIC_A_05_PROMPT_FIRST_CONNECT_THE_NETWORK);
						}
						else
						{
							color_led_flicker_stop();
							if (get_free_talk_flag() == FREE_TALK_STATE_FLAG_START_STATE)
							{
								color_led_off();
							}
							set_free_talk_flag(FREE_TALK_STATE_FLAG_STOP_STATE);
							//auto_play_pause();
							auto_play_stop();
							vTaskDelay(100);
							set_free_talk_pause();
							free_talk_set_mode(FREE_TALK_MODE_STOP);
							free_talk_stop();//关闭录音
							guo_xue_play_start();
							//free_talk_set_mode(FREE_TALK_MODE_STOP);
							set_functional_tyigger_key_state(FUNCTIONAL_TYIGGER_KEY_GUOXUE_STATE);
						}
					}
					break;
				}
				case DEVICE_NOTIFY_KEY_K4_TAP://睡眠歌曲播放
				{
					if (get_functional_tyigger_key_state() == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_B_27_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
					}
					else
					{
						if (get_wifi_state() != WIFI_ON_STATE)
						{
							player_mdware_play_tone(FLASH_MUSIC_A_05_PROMPT_FIRST_CONNECT_THE_NETWORK);
						}
						else
						{
							color_led_flicker_stop();
							if (get_free_talk_flag() == FREE_TALK_STATE_FLAG_START_STATE)
							{
								color_led_off();
							}
							set_free_talk_flag(FREE_TALK_STATE_FLAG_STOP_STATE);
							//auto_play_pause();
							auto_play_stop();
							vTaskDelay(100);
							set_free_talk_pause();
							free_talk_set_mode(FREE_TALK_MODE_STOP);
							free_talk_stop();//关闭录音
							bedtime_story_play_start();
							//free_talk_set_mode(FREE_TALK_MODE_STOP);
							set_functional_tyigger_key_state(FUNCTIONAL_TYIGGER_KEY_BEDTIME_STORY_STATE);
						}
					}
					break;
				}
				case DEVICE_NOTIFY_KEY_K4_LONGPRESSED://睡眠时间选择
				{
					sleep_type_switch();
					break;
				}
				case DEVICE_NOTIFY_KEY_K5_TAP://本地
				{
					if (get_functional_tyigger_key_state() == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_B_27_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
					}
					else
					{
						color_led_flicker_stop();
						if (get_free_talk_flag() == FREE_TALK_STATE_FLAG_START_STATE)
						{
							color_led_off();
						}
						set_free_talk_flag(FREE_TALK_STATE_FLAG_STOP_STATE);
						//auto_play_pause();
						auto_play_stop();
						vTaskDelay(100);
						set_free_talk_pause();
						free_talk_set_mode(FREE_TALK_MODE_STOP);
						free_talk_stop();//关闭录音
						//set_free_talk_pause();	//停止语音对话
						//free_talk_set_mode(FREE_TALK_MODE_STOP);
						if (sd_card_status_detect() == 0)//0为无sd状态
						{
							sd_music_start();
						}
						else
						{
							flash_music_start();
						}
						set_functional_tyigger_key_state(FUNCTIONAL_TYIGGER_KEY_FLASH_SD_STATE);
					}
					break;
				}
				case DEVICE_NOTIFY_KEY_K6_PUSH://微聊按
				{
					if (get_functional_tyigger_key_state() == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_B_27_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
					}
					else
					{
						if (get_wifi_state() != WIFI_ON_STATE)
						{
							player_mdware_play_tone(FLASH_MUSIC_A_05_PROMPT_FIRST_CONNECT_THE_NETWORK);
						}
						else
						{
							player_mdware_play_tone(FLASH_MUSIC_E_01_KEY_PRESS);
							vTaskDelay(650);
							auto_play_pause();
							wechat_record_start();
						}
					}
					break;
				}
				case DEVICE_NOTIFY_KEY_K6_RELEASE://微聊松
				{
					if (get_functional_tyigger_key_state() == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_B_27_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
					}
					else
					{
						if (get_wifi_state() != WIFI_ON_STATE)
						{
							player_mdware_play_tone(FLASH_MUSIC_A_05_PROMPT_FIRST_CONNECT_THE_NETWORK);
						}
						else
						{
							wechat_record_stop();
							player_mdware_play_tone(FLASH_MUSIC_E_01_KEY_PRESS);
						}
					}
					break;
				}
				case DEVICE_NOTIFY_KEY_K7_TAP://播放收藏
				{
#if MODULE_COLLECTION_FUNCTION
					if (get_functional_tyigger_key_state() == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_B_27_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
					}
					else
					{
						if (get_wifi_state() != WIFI_ON_STATE)
						{
							player_mdware_play_tone(FLASH_MUSIC_A_05_PROMPT_FIRST_CONNECT_THE_NETWORK);
						}
						else
						{
							color_led_flicker_stop();
							if (get_free_talk_flag() == FREE_TALK_STATE_FLAG_START_STATE)
							{
								color_led_off();
							}
							set_free_talk_flag(FREE_TALK_STATE_FLAG_STOP_STATE);
							//auto_play_pause();
							auto_play_stop();
							vTaskDelay(100);
							set_free_talk_pause();
							free_talk_set_mode(FREE_TALK_MODE_STOP);
							free_talk_stop();//关闭录音
							play_function_start();
							set_functional_tyigger_key_state(FUNCTIONAL_TYIGGER_KEY_SAVE_PLAY_STATE);
						}
					}
#endif
					break;
				}
				case DEVICE_NOTIFY_KEY_K7_LONGPRESSED://收藏
				{
					if (get_functional_tyigger_key_state() == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_B_27_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
					}
					else
					{
#if MODULE_COLLECTION_FUNCTION
						save_function();
#endif
					}
					break;
				}
				case DEVICE_NOTIFY_KEY_K8_TAP://下一首
				{
					if (keyborad_key_cancle())
					{
						break;
					}
					
					player_mdware_play_tone(FLASH_MUSIC_NEXT_SONG);
					vTaskDelay(2*1000);
					auto_play_next();
					break;
				}
				case DEVICE_NOTIFY_KEY_K9_TAP://暂停/播放
				{
					auto_play_pause_resume();
					set_free_talk_pause();
					free_talk_set_mode(FREE_TALK_MODE_STOP);
					free_talk_stop();//关闭录音
					break;
				}
				case DEVICE_NOTIFY_KEY_K10_LONGPRESSED://WIFI
				{
					if (get_functional_tyigger_key_state() == FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE)
					{
						player_mdware_play_tone(FLASH_MUSIC_B_27_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST);
					}
					else
					{
						set_wifi_state(WIFI_OFF_STATE);
						player_mdware_play_tone(FLASH_MUSIC_A_01_PROMPT_THE_CONNECTION_NETWORK);
						set_functional_tyigger_key_state(FUNCTIONAL_TYIGGER_KEY_SET_WIFI_STATE);
						service->sendEvent((MediaService*) service, MEDIA_WIFI_SMART_CONFIG_REQUEST, NULL, 0);
					}
					break;
				}
				case DEVICE_NOTIFY_KEY_K11_LONGPRESSED://童锁
				{
#if MODULE_PREVENT_CHILD_TRIGGER_FUNCTION
					prevent_child_trigger();
#endif
					break;
				}
				case DEVICE_NOTIFY_KEY_K12_TAP://耳灯
				{
#if MODULE_EAR_LED_FUNCTION
					switch_color_led();
#endif
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

        ESP_LOGE(KEYBOARD_TAG, "ERROR creating task_key_event_process task! Out of memory?");
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

