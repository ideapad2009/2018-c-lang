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
#include "esp_wifi.h"
#include "WifiManager.h"
#include "collection_function.h"
#include "deep_sleep_manage.h"
#include "keyword_requested_item_manage.h"

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

void key_matrix_callback(DEVICE_KEY_EVENT_T key_event)
{
	char buf1[32] = {0};
	
	switch (key_event)
	{
		case DEVICE_KEY_CHAT_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_CHAT_START");
			break;
		}
		case DEVICE_KEY_CHAT_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_CHAT_STOP");
			break;
		}
		
		case DEVICE_KEY_PREV_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_PREV_START");
			break;
		}
		case DEVICE_KEY_PREV_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_PREV_STOP");
			break;
		}
	
		case DEVICE_KEY_PLAY_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_PLAY_START");
			break;
		}
		case DEVICE_KEY_PLAY_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_PLAY_STOP");
			break;
		}

		case DEVICE_KEY_NEXT_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_NEXT_START");
			break;
		}
		case DEVICE_KEY_NEXT_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_NEXT_STOP");
			break;
		}

		case DEVICE_KEY_VOL_DOWN_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_VOL_DOWN_START");
			break;
		}
		case DEVICE_KEY_VOL_DOWN_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_VOL_DOWN_STOP");
			break;
		}

		case DEVICE_KEY_VOL_UP_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_VOL_UP_START");
			break;
		}
		case DEVICE_KEY_VOL_UP_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_VOL_UP_STOP");
			break;
		}

		case DEVICE_KEY_WECHAT_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_WECHAT_START");
			break;
		}
		case DEVICE_KEY_WECHAT_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_WECHAT_STOP");
			break;
		}
		
		case DEVICE_KEY_ESP32_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_ESP32_START");
			break;
		}
		case DEVICE_KEY_ESP32_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_ESP32_STOP");
			break;
		}
		
		case DEVICE_KEY_COLLECTION_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_COLLECTION_START");
			break;
		}
		case DEVICE_KEY_COLLECTION_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_COLLECTION_STOP");
			break;
		}

		case DEVICE_KEY_PLAY_COLLECTION_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_PLAY_COLLECTION_START");
			break;
		}
		case DEVICE_KEY_PLAY_COLLECTION_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_PLAY_COLLECTION_STOP");
			break;
		}

		case DEVICE_KEY_PLAY_NEWS_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_PLAY_NEWS_START");
			break;
		}
		case DEVICE_KEY_PLAY_NEWS_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_PLAY_NEWS_STOP");
			break;
		}

		case DEVICE_KEY_WIFI_SET_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_WIFI_SET_START");
			break;
		}
		case DEVICE_KEY_WIFI_SET_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_WIFI_SET_STOP");
			break;
		}

		case DEVICE_KEY_CH2EN_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_CH2EN_START");
			break;
		}
		case DEVICE_KEY_CH2EN_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_CH2EN_STOP");
			break;
		}

		case DEVICE_KEY_EN2CH_START:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_EN2CH_START");
			break;
		}
		case DEVICE_KEY_EN2CH_STOP:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_EN2CH_STOP");
			break;
		}

		case DEVICE_KEY_LOW_POWER:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_LOW_POWER");
			break;
		}
		
		case DEVICE_KEY_ROBOT_OFF:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_ROBOT_OFF");
			break;
		}

		case DEVICE_KEY_MIC_IN:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_MIC_IN");
			break;
		}
		
		case DEVICE_KEY_MIC_OUT:
		{
			snprintf(buf1, sizeof(buf1), "DEVICE_KEY_MIC_OUT");
			break;
		}
		
		default:
			snprintf(buf1, sizeof(buf1), "unknow key num %d", key_event);
			return;
			break;
	}

	ESP_LOGE(LOG_TAG, "%s", buf1);

	if (g_keyboard_service_handle.msg_queue != NULL && get_ota_upgrade_state() != OTA_UPGRADE_STATE_UPGRADING)
	{
		xQueueSend(g_keyboard_service_handle.msg_queue, &key_event, 0);
	}
}

void nlp_result_cb(NLP_RESULT_T *result)
{
	printf("chat_result=[%s]\r\n", result->chat_result.text);
}

int keyborad_key_cancle(void)
{
	static uint32_t key_press_time = 0;
	uint64_t time_now = get_time_of_day();
	
	if (abs(key_press_time - time_now) <= 1000)
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
    DEVICE_KEY_EVENT_T key_event = DEVICE_KEY_START;
    KEYBOARD_SERVICE_HANDLE_T *handle = (KEYBOARD_SERVICE_HANDLE_T *) pv;
	MediaService *service = handle->service;
	int chat_state = CHAT_CLOSE;
	static int low_power_tone_flag = 0;//0,ESP32状态下；1，非ESP32装态下
	PlayerStatus player_status;
		
	key_matrix_handle_t key_handle = adc_keyboard_matrix_init(key_matrix_callback, set_battery_vol);
	uart_esp32_status_mode(UART_ESP32_STATUS_MODE_DEVICE_START_STATU);//向MCU发送ESP32启动状态
    while (1) 
	{
		if (xQueueReceive(handle->msg_queue, &key_event, portMAX_DELAY) == pdPASS) 
		{
			update_device_sleep_time();
			switch (key_event)
			{
				case DEVICE_KEY_CHAT_START:
				{//语音对话
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_CHAT_START");
					if (service->_blocking == 0)
					{						
						free_talk_stop();
						auto_play_stop();
						vTaskDelay(200);
						
						player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);
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

				case DEVICE_KEY_PREV_START:	
				{//上一曲
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_PREV_START");
					if (keyborad_key_cancle())
					{
						break;
					}
					EspAudioStatusGet(&player_status);
					if (player_status.status == AUDIO_STATUS_PLAYING || player_status.status == AUDIO_STATUS_PAUSED)
					{
						uart_esp32_status_mode(UART_ESP32_STATUS_MODE_MUSIC_PLAYING);//向mcu发送播放状态
						player_mdware_play_tone(FLASH_MUSIC_PREV_SONG);
						vTaskDelay(2*1000);
						auto_play_prev();
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_NO_MUSIC);
					}
					break;
				}

				case DEVICE_KEY_PLAY_START:	
				{//暂停/播放
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_PLAY_START");
					if (!keyborad_key_cancle())
					{
						free_talk_stop();
						auto_play_pause_resume();
					}
					break;
				}
				
				case DEVICE_KEY_NEXT_START:	
				{//下一曲
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_NEXT_START");
					if (keyborad_key_cancle())
					{
						break;
					}
					EspAudioStatusGet(&player_status);
					if (player_status.status == AUDIO_STATUS_PLAYING || player_status.status == AUDIO_STATUS_PAUSED)
					{
						uart_esp32_status_mode(UART_ESP32_STATUS_MODE_MUSIC_PLAYING);//向mcu发送播放状态
						player_mdware_play_tone(FLASH_MUSIC_NEXT_SONG);
						vTaskDelay(2*1000);
						auto_play_next();
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_NO_MUSIC);
					}
					break;
				}
				
				case DEVICE_KEY_VOL_DOWN_START:
				{//音量减
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_VOL_DOWN_START");
                    int vol;
					
                    if (MediaHalGetVolume(&vol) == 0) 
					{
                        if (MediaHalSetVolume(vol - VOLUME_STEP) == 0) 
						{
                            ESP_LOGI(LOG_TAG, "current vol = %d (line %d)",
                                     vol - VOLUME_STEP < 0 ? vol : vol - VOLUME_STEP, __LINE__);
                        }
                    }
					break;
				}
				
				case DEVICE_KEY_VOL_UP_START:
				{//音量加
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_VOL_UP_START");
                    int vol;
                    if (MediaHalGetVolume(&vol) == 0) 
					{
                        if (MediaHalSetVolume(vol + VOLUME_STEP) == 0)
						{
                            ESP_LOGI(LOG_TAG, "current vol = %d (line %d)",
                                     vol + VOLUME_STEP > 100 ? vol : vol + VOLUME_STEP, __LINE__);
                        }
                    }

					if (vol + VOLUME_STEP > 100)
					{
						PlayerStatus player = {0};
		                service->getPlayerStatus((MediaService*)service, &player);
		                if (player.status != AUDIO_STATUS_PLAYING) 
						{
							player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);
							vTaskDelay(300);
		                } 
					}
					break;
				}
				
				case DEVICE_KEY_WECHAT_START:
				{//微聊
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_WECHAT_START");
					if (keyborad_key_cancle())
					{
						break;
					}
					
					if (service->_blocking == 0)
					{
						chat_state = CHAT_OPEN;
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
				
				case DEVICE_KEY_WECHAT_STOP:
				{//微聊
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_WECHAT_STOP");
					if (chat_state == CHAT_OPEN)
					{
						free_talk_stop();
						wechat_record_stop();
						chat_state = CHAT_CLOSE;
					}
					break;
				}

				case DEVICE_KEY_ESP32_START:
				{//启动ESP32模块
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_ESP32_START");
					low_power_tone_flag = 0;//0，ESP32状态下
					MediaHalPaPwr(1);
					MediaHalSetVolume(68);
					player_mdware_play_tone(FLASH_MUSIC_WIFI_MODE);
					vTaskDelay(1000);
					wifi_auto_re_connect_start();
					break;
				}
				
				case DEVICE_KEY_ESP32_STOP:
				{//暂停ESP32模块
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_ESP32_STOP");
					low_power_tone_flag = 1;//1，非ESP32状态下
					MediaHalPaPwr(0);
					MediaHalSetVolume(0);
					if (is_free_talk_running())
					{
						free_talk_stop();
					}
					auto_play_stop();
    				vTaskDelay(1500);
					wifi_off_start();
    				vTaskDelay(1000);
					break;
				}

				case DEVICE_KEY_COLLECTION_START:
				{//收藏
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_COLLECTION_START");
					save_function();//save playing url of music
					break;
				}
				
				case DEVICE_KEY_PLAY_COLLECTION_START:
				{//播放收藏
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_PLAY_COLLECTION_START");
					if (keyborad_key_cancle())
					{
						break;
					}
					if (is_free_talk_running())//if free_talk starts,free_talk will stop
					{
						free_talk_stop();
					}
					play_function_start();//play collection
					break;
				}

				case DEVICE_KEY_PLAY_NEWS_START:
				{
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_PLAY_NEWS_START");
					if (keyborad_key_cancle())
					{
						break;
					}
					if (is_free_talk_running())//if free_talk starts,free_talk will stop
					{
						free_talk_stop();
					}
					news_request_start();
					break;
				}
				case DEVICE_KEY_WIFI_SET_START:	
				{//配网
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_WIFI_SET_START");
					uart_esp32_status_mode(UART_ESP32_STATUS_MODE_WIFI_SETTING);//向mcu发送启动APP配网状态
#if 0
					led_ctrl_set_mode(LED_CTRL_EYE_FLASHING_SLOW);
					service->sendEvent((MediaService*) service, MEDIA_WIFI_SMART_CONFIG_REQUEST, NULL, 0);
#endif
					wifi_smart_connect_start();
					player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_CONFIG);
					break;
				}
				
				case DEVICE_KEY_CH2EN_START:	
				{//中译英
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_CH2EN_START");
					if (service->_blocking == 0)
					{						
						free_talk_stop();
						auto_play_stop();
						vTaskDelay(200);

						player_mdware_play_tone(FLASH_MUSIC_CHINESE_ENGLISH_TRANSLATE);
						free_talk_set_state(FREE_TALK_STATE_TRANSLATE_CH_EN);
						vTaskDelay(2000);
	
						free_talk_start();
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE);
						vTaskDelay(3*1000);
					}
					break;
				}
				
				case DEVICE_KEY_EN2CH_START:
				{//英译中
					ESP_LOGE(LOG_TAG, "DEVICE_KEY_EN2CH_START");
					if (service->_blocking == 0)
					{	
						free_talk_stop();
						auto_play_stop();
						vTaskDelay(200);
						player_mdware_play_tone(FLASH_MUSIC_ENGLISH_CHINESE_TRANSLATE);
						free_talk_set_state(FREE_TALK_STATE_TRANSLATE_EN_CH);
						vTaskDelay(2000);
						free_talk_start();
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE);
						vTaskDelay(3*1000);
					}
					break;
				}
				case DEVICE_KEY_LOW_POWER:
				{//低电播报
					printf("in DEVICE_KEY_LOW_POWER\n");
					if (low_power_tone_flag == 0)
					{
						player_mdware_play_tone(FLASH_MUSIC_BATTERY_LOW);
					}
					break;
				}
				case DEVICE_KEY_ROBOT_OFF:
				{//休眠
					printf("in DEVICE_KEY_ROBOT_OFF\n");
					EspAudioStop();
					if (low_power_tone_flag == 0)
					{
						player_mdware_play_tone(FLASH_MUSIC_POWER_OFF);
						vTaskDelay(4*1000);
					}
					deep_sleep_manage();
					break;
				}
				case DEVICE_KEY_MIC_IN:
				{//mic插入
					printf("in DEVICE_KEY_MIC_IN\n");
					g_mic_flag = 1;
					player_mdware_play_tone(FLASH_MUSIC_MICMODE);
					vTaskDelay(2000);
					g_mic_flag = 0;
					break;
				}
				case DEVICE_KEY_MIC_OUT:
				{//mic拔出
					printf("in DEVICE_KEY_MIC_OUT\n");
					g_mic_flag = 1;
					player_mdware_play_tone(FLASH_MUSIC_MICEXIT);
					vTaskDelay(2000);
					g_mic_flag = 0;
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

