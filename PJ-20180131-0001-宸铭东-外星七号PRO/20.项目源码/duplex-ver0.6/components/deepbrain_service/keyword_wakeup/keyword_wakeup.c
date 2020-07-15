#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "DeviceCommon.h"
#include "MediaControl.h"
#include "MediaHal.h"
#include "device_api.h"
#include "EspAudio.h"
#include "userconfig.h"
#include "player_middleware.h"
#include "keyword_wakeup.h"
#include "debug_log.h"
#include "auto_play_service.h"
#include "speech_wakeup.h"

//#define DEBUG_PRINT
#define LOG_TAG "keyword wakeup"

#define KWW_MAX_RECORD_TIME_MS	(30*24*60*60*1000)
#define KWW_MAX_CALLBACK_NUM	(10)

typedef enum
{
	KWW_MSG_RECORD_START,
	KWW_MSG_RECORD_READ,
	KWW_MSG_RECORD_STOP,
	KWW_MSG_QUIT,
}KWW_MSG_TYPE_T;

//录音数据
typedef struct 
{
	int record_id;
	int record_len;
	int record_ms;
	int is_max_ms;
	char data[AUDIO_RECORD_BUFFER_SIZE];
}KWW_RECORD_DATA_T;

//自由对话消息
typedef struct
{
	KWW_MSG_TYPE_T msg_type;
	KWW_RECORD_DATA_T *p_record_data;
}KEYWORD_WAKEUP_MSG_T;

//唤醒状态
typedef struct
{
	xQueueHandle msg_queue;
	MediaService *service;
	void *wakeup_handle;
	keyword_wakeup_event_cb callback_array[KWW_MAX_CALLBACK_NUM];
}KEYWORD_WAKEUP_HANDLE_T;

static KEYWORD_WAKEUP_HANDLE_T *g_keyword_wakeup_handle = NULL;

static void set_kww_record_data(
	int record_sn, int id, char *data, size_t data_len, int record_ms, int is_max)
{
	KEYWORD_WAKEUP_MSG_T msg = {0};

	if (g_keyword_wakeup_handle == NULL || g_keyword_wakeup_handle->msg_queue == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_kww_record_data invalid parmas");
		return;
	}

	msg.p_record_data = (KWW_RECORD_DATA_T *)esp32_malloc(sizeof(KWW_RECORD_DATA_T));
	if (msg.p_record_data == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_kww_record_data esp32_malloc failed");
		return;
	}

	memset(msg.p_record_data, 0, sizeof(KWW_RECORD_DATA_T));
	if (id == 1)
	{
		msg.msg_type = KWW_MSG_RECORD_START;
	}
	else if (id > 1)
	{
		msg.msg_type = KWW_MSG_RECORD_READ;
	}
	else
	{
		msg.msg_type = KWW_MSG_RECORD_STOP;	
	}

	msg.p_record_data->record_id = id;
	msg.p_record_data->record_len = data_len;
	msg.p_record_data->record_ms = record_ms;
	msg.p_record_data->is_max_ms = is_max;

	if (data_len > 0)
	{
		memcpy(msg.p_record_data->data, data, data_len);
	}

	if (xQueueSend(g_keyword_wakeup_handle->msg_queue, &msg, 0) != pdPASS)
	{
		ESP_LOGE(LOG_TAG, "xQueueSend g_keyword_wakeup_handle->msg_queue failed");
		esp32_free(msg.p_record_data);
		msg.p_record_data = NULL;
	}
}

static void kww_rec_start(void)
{
	player_mdware_start_record(1, KWW_MAX_RECORD_TIME_MS, set_kww_record_data);
}

static void kww_rec_stop(void)
{
	player_mdware_stop_record(set_kww_record_data);
}

static void kww_event_trigger(void)
{
	int i = 0;
	
	if (g_keyword_wakeup_handle == NULL)
	{
		return;
	}

	//遍历是否已经存在
	for (i=0; i<KWW_MAX_CALLBACK_NUM; i++)
	{
		if (g_keyword_wakeup_handle->callback_array[i] != NULL)
		{
			g_keyword_wakeup_handle->callback_array[i](KWW_EVENT_WAKEUP);
		}
	}

	return;
}


static void task_keyword_wakeup(void *pv)
{
	int ret = 0;
	int i = 0;
	int wakeup_count = 0;
	KEYWORD_WAKEUP_HANDLE_T *keyword_handle = (KEYWORD_WAKEUP_HANDLE_T *)pv;
	MediaService *service = keyword_handle->service;
	KEYWORD_WAKEUP_MSG_T msg = {0};

	//有网络或者15秒超时退出循环
	for (i=0; i<30; i++)
	{
		if (service->_blocking == 0)
		{
			vTaskDelay(1000/portTICK_RATE_MS);
			break;
		}
		vTaskDelay(500/portTICK_RATE_MS);
	}

	//创建唤醒词句柄
	if (keyword_handle->wakeup_handle == NULL)
	{
		YQSW_SetCustomAddr(KWW_AUTH_ADDR);
		YQSW_CreateChannel(&keyword_handle->wakeup_handle, "none");
		if (keyword_handle->wakeup_handle != NULL)
		{
			YQSW_ResetChannel(keyword_handle->wakeup_handle);	
		}
		else
		{
			ESP_LOGE(LOG_TAG, "task_keyword_wakeup YQSW_CreateChannel failed");
			return;
		}
	}

	keyword_wakeup_start();
	while (1) 
	{		
		if (xQueueReceive(keyword_handle->msg_queue, &msg, portMAX_DELAY) == pdPASS) 
		{			
#ifdef DEBUG_PRINT
			if (msg.p_record_data != NULL)
			{
				DEBUG_LOGE(LOG_TAG, "id=%d, len=%d, time=%dms", 
					msg.p_record_data->record_id, msg.p_record_data->record_len, msg.p_record_data->record_ms);
			}
#endif		
			switch (msg.msg_type)
			{
				case KWW_MSG_RECORD_START:
				{
					if (keyword_handle->wakeup_handle != NULL)
					{
						YQSW_ResetChannel(keyword_handle->wakeup_handle);
					}
				}
				case KWW_MSG_RECORD_READ:
				{
					int frame_len = msg.p_record_data->record_len;
					int frame_start = 0;

					//唤醒词唤醒
					while (frame_len >= AUDIO_PCM_10MS_FRAME_SIZE)
					{
						ret = YQSW_ProcessWakeup(keyword_handle->wakeup_handle, (int16_t*)(msg.p_record_data->data + frame_start), AUDIO_PCM_10MS_FRAME_SIZE/2);
						if (ret == 1)
						{
							wakeup_count++;
							kww_event_trigger();
							ESP_LOGE(LOG_TAG, "keyword wakeup now,count[%d]", wakeup_count);
							YQSW_ResetChannel(keyword_handle->wakeup_handle);
							break;
						}
						frame_len -= AUDIO_PCM_10MS_FRAME_SIZE;
						frame_start += AUDIO_PCM_10MS_FRAME_SIZE;
					}			
					break;
				}
				case KWW_MSG_RECORD_STOP:
				{
					break;
				}
				default:
					break;
			}

			if (msg.msg_type == KWW_MSG_QUIT)
			{
				if (msg.p_record_data != NULL)
				{
					esp32_free(msg.p_record_data);
					msg.p_record_data = NULL;
				}
				break;
			}

			if (msg.p_record_data != NULL)
			{
				esp32_free(msg.p_record_data);
				msg.p_record_data = NULL;
			}
		}
	}

	//释放唤醒词句柄
	if (keyword_handle->wakeup_handle != NULL)
	{
		YQSW_DestroyChannel(keyword_handle->wakeup_handle);
		keyword_handle->wakeup_handle = NULL;
	}

	//释放所有句柄
	esp32_free(keyword_handle);
	keyword_handle = NULL;
	
	vTaskDelete(NULL);
}

void keyword_wakeup_create(MediaService *self)
{
	g_keyword_wakeup_handle = (KEYWORD_WAKEUP_HANDLE_T *)esp32_malloc(sizeof(KEYWORD_WAKEUP_HANDLE_T));
	if (g_keyword_wakeup_handle == NULL)
	{
		return;
	}
	memset(g_keyword_wakeup_handle, 0, sizeof(KEYWORD_WAKEUP_HANDLE_T));

	g_keyword_wakeup_handle->service = self;
	g_keyword_wakeup_handle->msg_queue = xQueueCreate(20, sizeof(KEYWORD_WAKEUP_MSG_T));
	
	if (xTaskCreate(task_keyword_wakeup,
					"task_keyword_wakeup",
					512*6,
					g_keyword_wakeup_handle,
					4,
					NULL) != pdPASS) {

		ESP_LOGE(LOG_TAG, "ERROR creating task_keyword_wakeup task! Out of memory?");
	}
}

void keyword_wakeup_delete(void)
{
	KEYWORD_WAKEUP_MSG_T msg = {0};
	msg.msg_type = KWW_MSG_QUIT;
	
	if (g_keyword_wakeup_handle != NULL && g_keyword_wakeup_handle->msg_queue != NULL)
	{
		xQueueSend(g_keyword_wakeup_handle->msg_queue, &msg, 0);
	}
}

int keyword_wakeup_register_callback(keyword_wakeup_event_cb cb)
{
	int i = 0;
	
	if (g_keyword_wakeup_handle == NULL)
	{
		return 0;
	}

	//遍历是否已经存在
	for (i=0; i<KWW_MAX_CALLBACK_NUM; i++)
	{
		if (g_keyword_wakeup_handle->callback_array[i] == cb)
		{
			return 1;
		}
	}

	//插入新的
	for (i=0; i<KWW_MAX_CALLBACK_NUM; i++)
	{
		if (g_keyword_wakeup_handle->callback_array[i] == NULL)
		{
			g_keyword_wakeup_handle->callback_array[i] = cb;
			return 1;
		}
	}

	return 0;
}

int keyword_wakeup_cancel_callback(keyword_wakeup_event_cb cb)
{
	int i = 0;
	
	if (g_keyword_wakeup_handle == NULL)
	{
		return 0;
	}

	//遍历是否已经存在
	for (i=0; i<KWW_MAX_CALLBACK_NUM; i++)
	{
		if (g_keyword_wakeup_handle->callback_array[i] == cb)
		{
			g_keyword_wakeup_handle->callback_array[i] = NULL;
			return 1;
		}
	}

	return 0;
}


void keyword_wakeup_start(void)
{
	kww_rec_start();
}

void keyword_wakeup_stop(void)
{
	kww_rec_stop();
}

