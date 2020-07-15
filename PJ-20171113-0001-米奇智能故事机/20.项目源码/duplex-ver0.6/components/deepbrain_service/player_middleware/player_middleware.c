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

#include "cJSON.h"
#include "touchpad.h"
#include "DeviceCommon.h"
#include "MediaControl.h"
#include "MediaHal.h"
#include "toneuri.h"
#include "http_api.h"
#include "device_api.h"
#include "socket.h"
#include "lwip/netdb.h"
#include "EspAudio.h"
#include "userconfig.h"
#include "Recorder.h"
#include "player_middleware.h"

#define PLAY_TONE_WRITE_SIZE  		(1024*2)
#define TRANSLATE_RESULT_BUFFER_SIZE (500*1024)
#define PLAY_AUDIO_PERIOD_TIME		500
#define MAX_REC_SHARE_NUM			10

#define TAG_PLAYER_MDWARE "PLAYER MIDDLERWARE"

// audio record
typedef enum
{
	AUDIO_RAW_READ_STATUS_INIT,
	AUDIO_RAW_READ_STATUS_IDEL,
	AUDIO_RAW_READ_STATUS_START,
	AUDIO_RAW_READ_STATUS_READING,
	AUDIO_RAW_READ_STATUS_STOP,
}AUDIO_RAW_READ_STATUS_T;

typedef enum
{	
	PLAY_TONE_STATUS_IDEL = 0,
	PLAY_TONE_STATUS_START,
	PLAY_TONE_STATUS_PLAY,
	PLAY_TONE_STATUS_STOP,
}PLAY_TONE_STATUS_T;

typedef struct
{
	int data_len;
	char data[TRANSLATE_RESULT_BUFFER_SIZE];
}TRANSLATE_RESULT_BUFF_T;

//录音数据分享
typedef struct
{
	int record_sn;		//录音sn
	int record_index;	//录音索引
	uint64_t record_max_time;//录音最大时间
	uint64_t record_total_ms;//录音总时间
	player_mdware_record_cb call_back;
}REC_SHARE_INFO_T;

//录音数据分享
typedef struct
{
	char rec_data[AUDIO_RECORD_BUFFER_SIZE];
	REC_SHARE_INFO_T REC_SHARE[MAX_REC_SHARE_NUM];
}DATA_SHARE_STREAM_T;

PLAYER_MDWARE_CONFIG_T g_player_mdware_config = {0};
TRANSLATE_RESULT_BUFF_T *g_translate_buffer = NULL;
static SemaphoreHandle_t g_lock_middleware_player = NULL;
static uint64_t g_audio_start_time = 0;

static void create_lock(void)
{
	g_lock_middleware_player = xSemaphoreCreateMutex();
}

static void try_lock(void)
{
	if (g_lock_middleware_player != NULL)
	{
		xSemaphoreTake(g_lock_middleware_player, portMAX_DELAY);
	}
}

static void try_unlock(void)
{
	if (g_lock_middleware_player != NULL)
	{
		xSemaphoreGive(g_lock_middleware_player);
	}
}

static void destroy_lock(void)
{
	if (g_lock_middleware_player != NULL)
	{
		vSemaphoreDelete(g_lock_middleware_player);
	}
}

int get_player_mdware_status(void)
{
	return g_player_mdware_config.cur_status;
}

void set_player_mdware_status(int status)
{
	g_player_mdware_config.last_status = g_player_mdware_config.cur_status;
	g_player_mdware_config.cur_status = status;
}

int get_player_mdware_last_status(void)
{
	return g_player_mdware_config.last_status;
}

void player_mdware_play_tone(FLASH_MUSIC_INDEX_T flash_music_index)
{
	try_lock();
	uint64_t now = get_time_of_day();
	uint32_t time_pass = abs(now - g_audio_start_time);
	if (time_pass < PLAY_AUDIO_PERIOD_TIME)
	{
		vTaskDelay((PLAY_AUDIO_PERIOD_TIME - time_pass)/portTICK_RATE_MS);
	}
	g_audio_start_time = get_time_of_day();
	EspAudioTonePlay(get_flash_music_name_ex(flash_music_index), TERMINATION_TYPE_NOW);
	try_unlock();
}

void player_mdware_play_audio(const char *uri)
{
	try_lock();
	uint64_t now = get_time_of_day();
	uint32_t time_pass = abs(now - g_audio_start_time);
	if (time_pass < PLAY_AUDIO_PERIOD_TIME)
	{
		vTaskDelay((PLAY_AUDIO_PERIOD_TIME - time_pass)/portTICK_RATE_MS);
	}
	g_audio_start_time = get_time_of_day();
	EspAudioPlay(uri);
	try_unlock();
}

void player_mdware_start_record(
	int record_sn, 
	uint64_t record_ms,
	player_mdware_record_cb cb)
{
	if (g_player_mdware_config.queue_record_msg == NULL)
	{
		return;
	}

	PLAYER_MDWARE_MSG_T *p_msg = (PLAYER_MDWARE_MSG_T *)esp32_malloc(sizeof(PLAYER_MDWARE_MSG_T));

	if (p_msg == NULL)
	{
		return;
	}
	memset(p_msg, 0, sizeof(PLAYER_MDWARE_MSG_T));
	p_msg->msg_type = PLAYER_MDWARE_MSG_RECORD_START;
	p_msg->msg_start_recrod.record_sn = record_sn;
	p_msg->msg_start_recrod.cb = cb;
	p_msg->msg_start_recrod.record_ms = record_ms;

	if (xQueueSend(g_player_mdware_config.queue_record_msg, &p_msg, 0) != pdPASS)
	{
		esp32_free(p_msg);
		p_msg = NULL;
	}
}


void player_mdware_stop_record(player_mdware_record_cb cb)
{
	if (g_player_mdware_config.queue_record_msg == NULL)
	{
		return;
	}

	PLAYER_MDWARE_MSG_T *p_msg = (PLAYER_MDWARE_MSG_T *)esp32_malloc(sizeof(PLAYER_MDWARE_MSG_T));

	if (p_msg == NULL)
	{
		return;
	}
	memset(p_msg, 0, sizeof(PLAYER_MDWARE_MSG_T));
	p_msg->msg_type = PLAYER_MDWARE_MSG_RECORD_STOP;
	p_msg->msg_start_recrod.cb = cb;

	if (xQueueSend(g_player_mdware_config.queue_record_msg, &p_msg, 0) != pdPASS)
	{
		esp32_free(p_msg);
		p_msg = NULL;
	}
}

#if DATA_STREAM_SHARE_ENABLE
static void task_record_audio(void *pv)
{	
	int ret = ESP_OK;
	int len = 0;
	int i = 0;
	PLAYER_MDWARE_MSG_START_RECORD_T record_params = {0};
	AUDIO_RAW_READ_STATUS_T status = AUDIO_RAW_READ_STATUS_INIT;
	DATA_SHARE_STREAM_T* data_share_stream = (DATA_SHARE_STREAM_T *)esp32_malloc(sizeof(DATA_SHARE_STREAM_T));
	PLAYER_MDWARE_MSG_T *p_msg = NULL; 
	
    while (1) 
	{		
		BaseType_t xStatus = xQueueReceive(g_player_mdware_config.queue_record_msg, &p_msg, (TickType_t)20);
		if (xStatus == pdPASS)
		{
			switch (p_msg->msg_type)
			{
				case PLAYER_MDWARE_MSG_RECORD_START:
				{
					if (p_msg->msg_start_recrod.cb == NULL)
					{
						break;
					}

					int is_find = 0;
					//找到相同的
					for (i=0; i<MAX_REC_SHARE_NUM; i++)
					{
						REC_SHARE_INFO_T *rec_info = &data_share_stream->REC_SHARE[i]; 
						if (p_msg->msg_start_recrod.cb == rec_info->call_back)
						{
							rec_info->record_index++;
							rec_info->call_back(rec_info->record_sn, rec_info->record_index*(-1), NULL, 0, rec_info->record_total_ms, 0);
							rec_info->record_sn = p_msg->msg_start_recrod.record_sn;
							rec_info->record_max_time = p_msg->msg_start_recrod.record_ms;
							rec_info->record_index = 0;
							rec_info->record_total_ms = 0;
							is_find = 1;
							break;
						}
					}

					if (is_find)
					{
						break;
					}

					//插入新的
					for (i=0; i<MAX_REC_SHARE_NUM; i++)
					{
						REC_SHARE_INFO_T *rec_info = &data_share_stream->REC_SHARE[i]; 
						if (rec_info->call_back == NULL)
						{
							rec_info->record_sn = p_msg->msg_start_recrod.record_sn;
							rec_info->record_max_time = p_msg->msg_start_recrod.record_ms;
							rec_info->call_back = p_msg->msg_start_recrod.cb;
							rec_info->record_index = 0;
							rec_info->record_total_ms = 0;
							break;
						}
					}					
					
					break;
				}
				case PLAYER_MDWARE_MSG_RECORD_STOP:
				{
					if (p_msg->msg_start_recrod.cb == NULL)
					{
						break;
					}

					//找到相同的
					for (i=0; i<MAX_REC_SHARE_NUM; i++)
					{
						REC_SHARE_INFO_T *rec_info = &data_share_stream->REC_SHARE[i]; 
						if (p_msg->msg_start_recrod.cb == rec_info->call_back)
						{
							rec_info->record_index++;
							rec_info->call_back(rec_info->record_sn, rec_info->record_index*(-1), NULL, 0, rec_info->record_total_ms, 0);
							memset(rec_info, 0 ,sizeof(REC_SHARE_INFO_T));
							break;
						}
					}
					break;
				}
				default:
					break;
			}
	
			esp32_free(p_msg);
			p_msg = NULL;
		}

		switch (status)
		{
			case AUDIO_RAW_READ_STATUS_INIT:
			{
				int ret = EspAudioRecorderStart("i2s://16000:1@record.pcm#raw", 10 * 1024, 0);
				if (AUDIO_ERR_NO_ERROR != ret) 
				{
					ESP_LOGE(TAG_PLAYER_MDWARE, "### EspAudioRecorderStart failed, error=%d ###", ret);
					vTaskDelay(1000);
					continue;
				}
				status = AUDIO_RAW_READ_STATUS_READING;
			}
			case AUDIO_RAW_READ_STATUS_READING:
			{
				len = 0;
				ret = EspAudioRecorderRead((unsigned char*)data_share_stream->rec_data, AUDIO_RECORD_BUFFER_SIZE, &len);
				if (AUDIO_ERR_NOT_READY == ret) 
				{
					ESP_LOGE(TAG_PLAYER_MDWARE, "###EspAudioRecorderRead failed, error=%d ###", ret);
					break;
				}

				//ESP_LOGI(TAG_PLAYER_MDWARE, "EspAudioRecorderRead success, len=%d!", len);
				if (len > 0)
				{
					for (i=0; i<MAX_REC_SHARE_NUM; i++)
					{
						REC_SHARE_INFO_T *rec_info = &data_share_stream->REC_SHARE[i]; 
						if (rec_info->call_back != NULL)
						{
							rec_info->record_index++;
							rec_info->record_total_ms += len/AUDIO_PCM_ONE_MS_BYTE;

							if (rec_info->record_total_ms >= rec_info->record_max_time)
							{
								rec_info->call_back(rec_info->record_sn, rec_info->record_index, data_share_stream->rec_data, len, rec_info->record_total_ms, 1);
								rec_info->record_index++;
								rec_info->call_back(rec_info->record_sn, rec_info->record_index*(-1), NULL, 0, rec_info->record_total_ms, 0);
								memset(rec_info, 0 ,sizeof(REC_SHARE_INFO_T));
							}
							else
							{
								rec_info->call_back(rec_info->record_sn, rec_info->record_index, data_share_stream->rec_data, len, rec_info->record_total_ms, 0);
							}
						}
					}
				}
				break;
			}
			case AUDIO_RAW_READ_STATUS_STOP:
			{
				status = AUDIO_RAW_READ_STATUS_READING;
				break;
			}
			default:
				break;
		}
    }

	if (data_share_stream != NULL)
	{
		esp32_free(data_share_stream);
		data_share_stream = NULL;
	}
	
    vTaskDelete(NULL);
}

#else
static void task_record_audio(void *pv)
{	
	int ret = ESP_OK;
	int len = 0;
	int record_id = 0;
	int record_ms = 0;
	PLAYER_MDWARE_MSG_START_RECORD_T record_params = {0};
	AUDIO_RAW_READ_STATUS_T status = AUDIO_RAW_READ_STATUS_INIT;
	char* audio_raw_data = (char*)esp32_malloc(AUDIO_RECORD_BUFFER_SIZE);
	PLAYER_MDWARE_MSG_T *p_msg = NULL; 
	
    while (1) 
	{		
		BaseType_t xStatus = xQueueReceive(g_player_mdware_config.queue_record_msg, &p_msg, (TickType_t)20);
		if (xStatus == pdPASS)
		{
			switch (p_msg->msg_type)
			{
				case PLAYER_MDWARE_MSG_RECORD_START:
				{
					//ESP_LOGE(TAG_PLAYER_MDWARE, "PLAYER_MDWARE_MSG_RECORD_START!");
					if (record_params.cb != NULL)
					{
						record_id++;
						record_params.cb(record_id*(-1), NULL, 0, record_ms, 0);
					}
					status = AUDIO_RAW_READ_STATUS_START;
					record_params = p_msg->msg_start_recrod;
					break;
				}
				case PLAYER_MDWARE_MSG_RECORD_STOP:
				{
					//ESP_LOGE(TAG_PLAYER_MDWARE, "PLAYER_MDWARE_MSG_RECORD_STOP!");
					if (status == AUDIO_RAW_READ_STATUS_READING
						|| status == AUDIO_RAW_READ_STATUS_START)
					{
						status = AUDIO_RAW_READ_STATUS_STOP;
					}
					break;
				}
				default:
					break;
			}
	
			esp32_free(p_msg);
			p_msg = NULL;
		}

		switch (status)
		{
			case AUDIO_RAW_READ_STATUS_INIT:
			{
				int ret = EspAudioRecorderStart("i2s://16000:1@record.pcm#raw", 10 * 1024, 0);
				if (AUDIO_ERR_NO_ERROR != ret) 
				{
					ESP_LOGE(TAG_PLAYER_MDWARE, "### EspAudioRecorderStart failed, error=%d ###", ret);
					vTaskDelay(1000);
					continue;
				}
				status = AUDIO_RAW_READ_STATUS_IDEL;
			}
			case AUDIO_RAW_READ_STATUS_IDEL:
			{
				break;
			}
			case AUDIO_RAW_READ_STATUS_START:
			{
				record_id = 0;
				record_ms = 0;
				status = AUDIO_RAW_READ_STATUS_READING;
				break;
			}
			case AUDIO_RAW_READ_STATUS_READING:
			{
				len = 0;
				ret = EspAudioRecorderRead((unsigned char*)audio_raw_data, AUDIO_RECORD_BUFFER_SIZE, &len);
				if (AUDIO_ERR_NOT_READY == ret) 
				{
					ESP_LOGE(TAG_PLAYER_MDWARE, "###EspAudioRecorderRead failed, error=%d ###", ret);
					break;
				}

				//ESP_LOGI(TAG_PLAYER_MDWARE, "EspAudioRecorderRead success, len=%d!", len);
				if (len > 0)
				{
					record_id++;
					record_ms += len/AUDIO_PCM_ONE_MS_BYTE;
					
					if (record_params.cb != NULL)
					{
						if (record_ms >= record_params.record_ms)
						{
							status = AUDIO_RAW_READ_STATUS_STOP;
							record_params.cb(record_id, audio_raw_data, len, record_ms, 1);
						}
						else
						{
							record_params.cb(record_id, audio_raw_data, len, record_ms, 0);
						}
					}
				}
				break;
			}
			case AUDIO_RAW_READ_STATUS_STOP:
			{
				if (record_params.cb != NULL)
				{
					record_id++;
					record_params.cb(record_id*(-1), NULL, 0, record_ms, 0);
				}
				record_params.cb = NULL;
				status = AUDIO_RAW_READ_STATUS_IDEL;
				break;
			}
			default:
				break;
		}
    }
	
    vTaskDelete(NULL);
}
#endif

void create_player_middleware(void)
{
	g_player_mdware_config.cur_status = PLAYER_MDWARE_STATUS_IDEL;
	g_player_mdware_config.last_status = PLAYER_MDWARE_STATUS_IDEL;
	g_player_mdware_config.queue_record_msg = xQueueCreate(10, sizeof(char *));
	create_lock();

	if (xTaskCreate(task_record_audio,
        "task_record_audio",
        512*5,
        NULL,
        4,
        NULL) != pdPASS) 
    {

        ESP_LOGE(TAG_PLAYER_MDWARE, "ERROR creating task_record_audio failed! Out of memory?");
    }
}

