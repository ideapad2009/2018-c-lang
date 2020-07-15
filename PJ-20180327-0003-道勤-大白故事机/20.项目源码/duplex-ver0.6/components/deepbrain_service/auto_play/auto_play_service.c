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
#include "led.h"
#include "auto_play_service.h"
#include "player_middleware.h"
#define LOG_TAG "autoplay service"
#define AUTO_PLAY_LIST_SIZE 2

typedef enum
{
	AUTO_PLAY_MSG_START,
	AUTO_PLAY_MSG_PREV,
	AUTO_PLAY_MSG_PLAY_PAUSE,
	AUTO_PLAY_MSG_PAUSE,
	AUTO_PLAY_MSG_NEXT,
	AUTO_PLAY_MSG_STOP,
	AUTO_PLAY_MSG_QUIT,
}AUTO_PLAY_MSG_T;

typedef struct
{
	int 	total_count;					//总数
	int 	cur_play_index;					//当前播放索引
	char 	list[AUTO_PLAY_LIST_SIZE][1024];//播放列表
}AUTO_PLAY_LIST_T;

typedef enum
{
	AUTO_PLAY_STATUS_PLAY,
	AUTO_PLAY_STATUS_PAUSE,
	AUTO_PLAY_STATUS_STOP,
}AUTO_PLAY_STATUS_T;

typedef struct 
{
	time_t last_play_time;
	AUTO_PLAY_STATUS_T status;
	xQueueHandle msg_queue;
	AUTO_PLAY_CALL_BACK_T call_back_list;
	AUTO_PLAY_MUSIC_SOURCE_T music_src;
	AUTO_PLAY_LIST_T play_list;
	BAIDU_ACOUNT_T baidu_acount;
	int play_error_count;
}AUTO_PLAY_HANDLE_T;

static AUTO_PLAY_HANDLE_T *g_auto_play_handle = NULL;
static SemaphoreHandle_t g_lock_auto_play = NULL;

static void create_lock(void)
{
	g_lock_auto_play = xSemaphoreCreateMutex();
}

static void try_lock(void)
{
	if (g_lock_auto_play != NULL)
	{
		xSemaphoreTake(g_lock_auto_play, portMAX_DELAY);
	}
}

static void try_unlock(void)
{
	if (g_lock_auto_play != NULL)
	{
		xSemaphoreGive(g_lock_auto_play);
	}
}

static void destroy_lock(void)
{
	if (g_lock_auto_play != NULL)
	{
		vSemaphoreDelete(g_lock_auto_play);
	}
}


void play_tts(char *str_tts)
{
	BAIDU_ACOUNT_T *baidu_acount;
	char *reply_buf;
	BAIDU_TTS_RESULT baiduRet;
	if (!(reply_buf = esp32_malloc(1024)))
		return;
	if (!(baidu_acount = esp32_malloc(sizeof (BAIDU_ACOUNT_T)))) 
	{
		esp32_free(reply_buf);
		return;
	}
	memset(reply_buf, 0, 1024);
	memset(baidu_acount, 0, sizeof(BAIDU_ACOUNT_T));
	get_ai_acount(AI_ACOUNT_BAIDU, baidu_acount);
	baiduRet = baidu_tts(baidu_acount, reply_buf, 1024, str_tts);
	if (BAIDU_TTS_SUCCESS == baiduRet)
	{
		printf("tts url:%s\r\n", reply_buf);
		EspAudioTonePlay(reply_buf, TERMINATION_TYPE_NOW);
		printf("tts url:%s\r\n", reply_buf);
	}
	esp32_free(reply_buf);
	esp32_free(baidu_acount);
}

static void play_music(AUTO_PLAY_HANDLE_T *handle)
{
	int i = 0;

	memset(&handle->play_list, 0, sizeof(handle->play_list));
	if (handle->music_src.need_play_name && strlen(handle->music_src.music_name) > 0)
	{
		get_ai_acount(AI_ACOUNT_BAIDU, &handle->baidu_acount);
		BAIDU_TTS_RESULT baiduRet = baidu_tts(&handle->baidu_acount, &handle->play_list.list[handle->play_list.total_count], 
			sizeof(handle->play_list.list[handle->play_list.total_count]), handle->music_src.music_name);
		if (BAIDU_TTS_SUCCESS == baiduRet)
		{
			handle->play_list.total_count++;
		}
	}
	
	if (strlen(handle->music_src.music_url) > 0)
	{
		snprintf(handle->play_list.list[handle->play_list.total_count], sizeof(handle->play_list.list[handle->play_list.total_count]), 
			"%s", handle->music_src.music_url);
		handle->play_list.total_count++;
	}

	handle->status = AUTO_PLAY_STATUS_STOP;
	if (handle->play_list.total_count > 0)
	{
		handle->play_error_count = 0;
		player_mdware_play_audio(handle->play_list.list[0]);
		handle->status = AUTO_PLAY_STATUS_PLAY;
	}

	for (i=0; i<handle->play_list.total_count; i++)
	{
		ESP_LOGE(LOG_TAG, "[%d][%s]", i+1, handle->play_list.list[i]);
	}

	if (handle->play_list.total_count == 0)
	{
		auto_play_stop();
	}
}

static void task_auto_play(void *pv)
{
    AUTO_PLAY_MSG_T msg = -1;
    AUTO_PLAY_HANDLE_T *handle = (AUTO_PLAY_HANDLE_T *) pv;
	PlayerStatus player_status;
	
    while (1) 
	{
		if (xQueueReceive(handle->msg_queue, &msg, (TickType_t)500) == pdPASS) 
		{
			//自动播放消息处理
			switch (msg)
			{
				case AUTO_PLAY_MSG_START:
				{
					if (handle->call_back_list.now)
					{
						handle->call_back_list.now(&handle->music_src);
						play_music(handle);
					}
					break;
				}
				case AUTO_PLAY_MSG_PREV:
				{
					if (handle->call_back_list.prev)
					{
						handle->call_back_list.prev(&handle->music_src);
						play_music(handle);
					}
					break;
				}
				case AUTO_PLAY_MSG_PLAY_PAUSE:
				{
					EspAudioStatusGet(&player_status);
					switch (player_status.status)
					{			
					    case AUDIO_STATUS_PLAYING:
						{
							EspAudioPause();
							player_mdware_play_tone(FLASH_MUSIC_MUSIC_PAUSE);
							vTaskDelay(2000);
							break;
						}
					    case AUDIO_STATUS_PAUSED:
						{
							player_mdware_play_tone(FLASH_MUSIC_MUSIC_PLAY);
							vTaskDelay(2000);
							EspAudioResume();
							break;
						}
						case AUDIO_STATUS_UNKNOWN:
					    case AUDIO_STATUS_STOP:
					    case AUDIO_STATUS_ERROR:
					    case AUDIO_STATUS_AUX_IN:
						{
							switch (handle->status)
							{
								case AUTO_PLAY_STATUS_STOP:
								{
									player_mdware_play_tone(FLASH_MUSIC_NO_MUSIC);
									break;
								}
								case AUTO_PLAY_STATUS_PLAY:
								{
									player_mdware_play_tone(FLASH_MUSIC_MUSIC_PAUSE);
									handle->status = AUTO_PLAY_STATUS_PAUSE;
									break;
								}
								case AUTO_PLAY_STATUS_PAUSE:
								{
									player_mdware_play_tone(FLASH_MUSIC_MUSIC_PLAY);
									handle->status = AUTO_PLAY_STATUS_PLAY;
									break;
								}
								default:
									break;
							}
						}
						default:
							break;
					}
					break;
				}
				case AUTO_PLAY_MSG_PAUSE:
				{
					switch (player_status.status)
					{			
					    case AUDIO_STATUS_PLAYING:
						{
							EspAudioPause();
							break;
						}
						case AUDIO_STATUS_UNKNOWN:
					    case AUDIO_STATUS_PAUSED:
					    case AUDIO_STATUS_STOP:
					    case AUDIO_STATUS_ERROR:
					    case AUDIO_STATUS_AUX_IN:
					    {
					    	break;
						}
						default:
							break;
					}
					handle->status = AUTO_PLAY_STATUS_PAUSE;
					break;
				}
				case AUTO_PLAY_MSG_NEXT:
				{
					if (handle->call_back_list.next)
					{
						handle->call_back_list.next(&handle->music_src);
						play_music(handle);
					}
					break;
				}
				case AUTO_PLAY_MSG_STOP:
				{
					try_lock();
					EspAudioToneStop();
					EspAudioStop();
					memset(&handle->call_back_list, 0, sizeof(handle->call_back_list));
					memset(&handle->music_src, 0, sizeof(handle->music_src));
					memset(&handle->play_list, 0, sizeof(handle->play_list));
					try_unlock();
					break;
				}
				case AUTO_PLAY_MSG_QUIT:
				{
					break;
				}
				default:
					break;
			}
		}

		if (msg == AUTO_PLAY_MSG_QUIT)
		{
			break;
		}
		
		//播放状态检测
		EspAudioStatusGet(&player_status);
		//ESP_LOGE(LOG_TAG, "[1]mode=%d, status=%d", player_status.mode, player_status.status);

		if (player_status.status == AUDIO_STATUS_ERROR)
		{
			if (strlen(handle->play_list.list[handle->play_list.cur_play_index]) > 0
				&& handle->play_error_count == 0)
			{
				player_mdware_play_audio(handle->play_list.list[handle->play_list.cur_play_index]);
			}
			
			if (handle->play_error_count == 1)
			{
				player_mdware_play_tone(FLASH_MUSIC_NO_MUSIC);
				auto_play_stop();
			}
			handle->play_error_count++;
			continue;
		}
		
		//非music播放模式，直接下一次
		if (player_status.mode > PLAYER_WORKING_MODE_MUSIC)
		{
			continue;
		}

		switch (player_status.status)
		{
		    case AUDIO_STATUS_PLAYING:
			{
				break;
			}
			case AUDIO_STATUS_ERROR:
			{
				break;
			}
		    case AUDIO_STATUS_PAUSED:
			{
				break;
			}
			case AUDIO_STATUS_UNKNOWN:
		    case AUDIO_STATUS_STOP:
			{
				static int count = 0;
				struct MusicInfo info = {0};
				EspAudioInfoGet(&info);
			
				if (handle->play_list.total_count == 0)
				{
					break;
				}

				//控制1秒后，切换至下一首
				count++;
				if (count < 1)
				{
					break;
				}
				count = 0;

				if (handle->status == AUTO_PLAY_STATUS_PAUSE)
				{
					break;
				}
				
				handle->play_list.cur_play_index++;
				if (handle->play_list.cur_play_index < handle->play_list.total_count)
				{
					player_mdware_play_audio(handle->play_list.list[handle->play_list.cur_play_index]);
				}
				else
				{
					handle->status = AUTO_PLAY_STATUS_STOP;
					if (handle->call_back_list.next)
					{
						handle->call_back_list.next(&handle->music_src);
						play_music(handle);
					}
				}
				
				break;
			}
		    case AUDIO_STATUS_AUX_IN:
			{
				break;
			}
			default:
				break;
		}			
    }
    vTaskDelete(NULL);
}

static void auto_play_send_msg(int msg)
{
	if (g_auto_play_handle == NULL || g_auto_play_handle->msg_queue == NULL)
	{
		return;
	}
	
	xQueueSend(g_auto_play_handle->msg_queue, &msg, 0);
}

void auto_play_start()
{
	auto_play_send_msg(AUTO_PLAY_MSG_START);
}

void auto_play_prev()
{
	auto_play_send_msg(AUTO_PLAY_MSG_PREV);
}

void auto_play_next()
{
	auto_play_send_msg(AUTO_PLAY_MSG_NEXT);
}

void auto_play_stop()
{
	auto_play_send_msg(AUTO_PLAY_MSG_STOP);
}

void auto_play_pause_resume()
{
	auto_play_send_msg(AUTO_PLAY_MSG_PLAY_PAUSE);
}

void auto_play_pause()
{
	auto_play_send_msg(AUTO_PLAY_MSG_PAUSE);
}

void set_auto_play_cb(AUTO_PLAY_CALL_BACK_T *call_back_list)
{
	try_lock();
	g_auto_play_handle->call_back_list = *call_back_list;
	try_unlock();
	auto_play_start();
	
}

void get_auto_play_cb(AUTO_PLAY_CALL_BACK_T *call_back_list)
{
	try_lock();
	if (call_back_list != NULL && g_auto_play_handle != NULL)
	{
		*call_back_list = g_auto_play_handle->call_back_list;
	}
	try_unlock();
	ESP_LOGE(LOG_TAG, "get_auto_play_cb failed");
}

void auto_play_service_create(void)
{
	g_auto_play_handle = (AUTO_PLAY_HANDLE_T*)esp32_malloc(sizeof(AUTO_PLAY_HANDLE_T));
	if (g_auto_play_handle == NULL)
	{
		ESP_LOGE(LOG_TAG, "g_auto_play_handle malloc failed");
		return;
	}
	memset(g_auto_play_handle, 0, sizeof(AUTO_PLAY_HANDLE_T));
	create_lock();
	
	g_auto_play_handle->msg_queue = xQueueCreate(2, sizeof(char *));
	if (g_auto_play_handle->msg_queue == NULL)
	{
		ESP_LOGE(LOG_TAG, "msg_queue create failed");
		
		esp32_free(g_auto_play_handle);
		g_auto_play_handle = NULL;
		
		return;
	}
	
    if (xTaskCreate(task_auto_play,
                    "task_auto_play",
                    1024*3,
                    g_auto_play_handle,
                    4,
                    NULL) != pdPASS) 
    {

        ESP_LOGE(LOG_TAG, "ERROR creating task_auto_play task! Out of memory?");
		
		vQueueDelete(g_auto_play_handle->msg_queue);
		g_auto_play_handle->msg_queue = NULL;
		
		esp32_free(g_auto_play_handle);
		g_auto_play_handle = NULL;
		
		return;
    }
}

void auto_play_service_delete(void)
{
	auto_play_send_msg(AUTO_PLAY_MSG_QUIT);
}

