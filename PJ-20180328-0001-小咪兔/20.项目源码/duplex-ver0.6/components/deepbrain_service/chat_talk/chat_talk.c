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
#include "wechat_service.h"
#include "player_middleware.h"
#include "interf_enc.h"
#include "mpush_service.h"
#include "bind_device.h"
#include "chat_talk.h"
#include "ReSampling.h"
#include "nlp_service.h"
#include "auto_play_service.h"

//#define DEBUG_PRINT
#define LOG_TAG "chat talk"

#define CHAT_TALK_AMR_MAX_TIME	10
#define CHAT_TALK_MAX_TIME_MS	(CHAT_TALK_AMR_MAX_TIME*1000)
#define CHAT_TALK_MAX_AMR_AUDIO_SIZE (CHAT_TALK_MAX_TIME_MS*AMR_20MS_AUDIO_SIZE/20 + 6)

typedef enum
{
	CHAT_TALK_MSG_RECORD_START,
	CHAT_TALK_MSG_RECORD_READ,
	CHAT_TALK_MSG_RECORD_STOP,
	CHAT_TALK_MSG_QUIT,
}CHAT_TALK_MSG_TYPE_T;

typedef struct
{
	int record_quiet_ms;
	int record_ms;
	int keyword_record_ms;
	int record_len;
	char record_data[CHAT_TALK_MAX_AMR_AUDIO_SIZE];
}CHAT_TALK_STATUS_T;

typedef struct
{
	int data_len;
	char data[AUDIO_RECORD_BUFFER_SIZE];
}CHAT_TALK_ENCODE_BUFF_T;

typedef struct 
{
	int record_id;
	int record_len;
	int record_ms;
	int is_max_ms;
	char data[AUDIO_RECORD_BUFFER_SIZE];
}CHAT_TALK_RECORD_DATA_T;

typedef struct
{
	char asr_result[512];
}CHAT_TALK_ASR_RESULT_T;

typedef struct
{
	xQueueHandle msg_queue;
	CHAT_TALK_STATUS_T status;
	void *amr_encode_handle;
	CHAT_TALK_ENCODE_BUFF_T pcm_encode;
	int cur_play_index;
	NLP_RESULT_T nlp_result;
	MediaService *service;
	BAIDU_ACOUNT_T baidu_acount;
	CHAT_TALK_ASR_RESULT_T asr_result;
}CHAT_TALK_HANDLE_T;


typedef struct
{
	CHAT_TALK_MSG_TYPE_T msg_type;
	CHAT_TALK_RECORD_DATA_T *p_record_data;
}CHAT_TALK_MSG_T;

static CHAT_TALK_HANDLE_T *g_chat_talk_handle = NULL;
static xSemaphoreHandle g_chat_talk_mux = NULL;

extern void chat_talk_nlp_result_cb(NLP_RESULT_T *result);

static void get_prev_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_chat_talk_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_chat_talk_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	if (g_chat_talk_handle->cur_play_index > 0)
	{
		g_chat_talk_handle->cur_play_index--;
	}

	ESP_LOGI(LOG_TAG, "get_prev_music [%d][%s][%s]", 
		g_chat_talk_handle->cur_play_index+1,
		p_links->link[g_chat_talk_handle->cur_play_index].link_name,
		p_links->link[g_chat_talk_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_chat_talk_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_chat_talk_handle->cur_play_index].link_url);
}


static void get_next_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_chat_talk_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_chat_talk_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	if (g_chat_talk_handle->cur_play_index < p_links->link_size - 1)
	{
		g_chat_talk_handle->cur_play_index++;
	}
	else if (g_chat_talk_handle->cur_play_index == p_links->link_size - 1)
	{
		if (strlen(g_chat_talk_handle->nlp_result.input_text) > 0 && p_links->link_size == NLP_SERVICE_LINK_MAX_NUM)
		{
			nlp_service_send_request(g_chat_talk_handle->nlp_result.input_text, chat_talk_nlp_result_cb);
		}
		return;
	}

	ESP_LOGI(LOG_TAG, "get_next_music [%d][%s][%s]", 
		g_chat_talk_handle->cur_play_index+1,
		p_links->link[g_chat_talk_handle->cur_play_index].link_name,
		p_links->link[g_chat_talk_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_chat_talk_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_chat_talk_handle->cur_play_index].link_url);
}

static void get_current_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_chat_talk_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_chat_talk_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	ESP_LOGI(LOG_TAG, "get_current_music [%d][%s][%s]", 
		g_chat_talk_handle->cur_play_index+1,
		p_links->link[g_chat_talk_handle->cur_play_index].link_name,
		p_links->link[g_chat_talk_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_chat_talk_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_chat_talk_handle->cur_play_index].link_url);
}

void chat_talk_nlp_result_cb(NLP_RESULT_T *result)
{
	NLP_RESULT_LINKS_T *p_links = &result->link_result;
	AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_music, get_next_music, get_current_music};

	if (g_chat_talk_handle == NULL)
	{
		return;
	}
	
	switch (result->type)
	{
		case NLP_RESULT_TYPE_NONE:
		{
			player_mdware_play_tone(FLASH_MUSIC_NOT_HEAR_CLEARLY_PLEASE_REPEAT);
			break;
		}
		case NLP_RESULT_TYPE_CHAT:
		{
			//play tts
			auto_play_pause();
			ESP_LOGE(LOG_TAG, "chat_result=[%s]\r\n", result->chat_result.text);
			play_tts(result->chat_result.text);
			break;
		}
		case NLP_RESULT_TYPE_LINK:
		{
			if (p_links->link_size > 0)
			{
				g_chat_talk_handle->nlp_result = *result;
				g_chat_talk_handle->cur_play_index = 0;
				set_auto_play_cb(&call_backs);
			}
			break;
		}
		default:
			break;
	}
}

static void chat_talk_amr_encode(
	CHAT_TALK_RECORD_DATA_T *p_record_data, CHAT_TALK_HANDLE_T *chat_talk_handle) 
{
	int ret = 0;
	int i  = 0;
	char amr_data_frame[AMR_20MS_AUDIO_SIZE] = {0};
	RESAMPLE resample = {0};
	
	if (p_record_data == NULL || chat_talk_handle == NULL)
	{
		return;
	}

	CHAT_TALK_STATUS_T *p_status = &chat_talk_handle->status;
	CHAT_TALK_ENCODE_BUFF_T *p_pcm_encode = &chat_talk_handle->pcm_encode;
	
	//降采样
	ret = unitSampleProc((short *)p_record_data->data, (short *)p_pcm_encode->data, 
		AUDIO_PCM_RATE, 8000, p_record_data->record_len, 1, &resample);
	if (ret > 0)
	{
		p_pcm_encode->data_len = ret;
		//ESP_LOGE(LOG_TAG, "task_wechat unitSampleProc success[%d]", ret);
	}
	else
	{
		ESP_LOGE(LOG_TAG, "chat_talk_amr_encode unitSampleProc failed[%d]", ret);
		return;
	}

	//转AMR
	for (i=0; (i*320) < p_pcm_encode->data_len; i++)
	{
		if ((p_status->record_len + sizeof(amr_data_frame)) > sizeof(p_status->record_data))
		{
			break;
		}
		
		ret = Encoder_Interface_Encode(chat_talk_handle->amr_encode_handle, MR122, 
			(short*)(p_pcm_encode->data + i*320), &amr_data_frame, 0);
		if (ret == sizeof(amr_data_frame))
		{
			memcpy(p_status->record_data + p_status->record_len, amr_data_frame, sizeof(amr_data_frame));
			p_status->record_len += sizeof(amr_data_frame);
			p_status->record_ms += 20;
		}
		else
		{
			ESP_LOGE(LOG_TAG, "chat_talk_amr_encode Encoder_Interface_Encode failed[%d]", ret);
		}
	}
}

static void task_chat_talk(void *pv)
{
	int ret = 0;
	int i = 0;
    DEVICE_KEY_EVENT_T key_event = DEVICE_KEY_START;
    CHAT_TALK_HANDLE_T *chat_talk_handle = (CHAT_TALK_HANDLE_T *)pv;
	MediaService *service = chat_talk_handle->service;
	CHAT_TALK_MSG_T msg = {0};

    while (1) 
	{
		if (chat_talk_handle->amr_encode_handle == NULL)
		{
			chat_talk_handle->amr_encode_handle = Encoder_Interface_init(0);
			if (chat_talk_handle->amr_encode_handle == NULL)
			{
				ESP_LOGE(LOG_TAG, "Encoder_Interface_init failed");
				vTaskDelay(1000);
				continue;
			}
		}
		
		if (xQueueReceive(chat_talk_handle->msg_queue, &msg, portMAX_DELAY) == pdPASS) 
		{			
#ifdef DEBUG_PRINT
			if (msg.p_record_data != NULL)
			{
				ESP_LOGI(LOG_TAG, "id=%d, len=%d, time=%dms", 
				msg.p_record_data->record_id, msg.p_record_data->record_len, msg.p_record_data->record_ms);
			}
#endif			
			switch (msg.msg_type)
			{
				case CHAT_TALK_MSG_RECORD_START:
				{
					snprintf(chat_talk_handle->status.record_data, sizeof(chat_talk_handle->status.record_data), "%s", AMR_HEADER);
					chat_talk_handle->status.record_len = strlen(AMR_HEADER);
				}
				case CHAT_TALK_MSG_RECORD_READ:
				{
					chat_talk_amr_encode(msg.p_record_data, chat_talk_handle);
					break;
				}
				case CHAT_TALK_MSG_RECORD_STOP:
				{				
					//语音小于1秒
					if (msg.p_record_data->record_ms < 1000)
					{
						//长按语音键 听到嘟声后 说出你想听的
						player_mdware_play_tone(FLASH_MUSIC_CHAT_SHORT_AUDIO);
						break;
					}

					player_mdware_play_tone(FLASH_MUSIC_MSG_SEND);
				
					//AMR语音识别
					get_ai_acount(AI_ACOUNT_BAIDU, &chat_talk_handle->baidu_acount);
					memset(&chat_talk_handle->asr_result, 0, sizeof(chat_talk_handle->asr_result));
#if	DEBUG_RECORD_SDCARD
					static int count = 1;
					FILE *p_record_file = NULL;
					char str_record_path[32] = {0};
					snprintf(str_record_path, sizeof(str_record_path), "/sdcard/1-%d.amr", count++);
					p_record_file = fopen(str_record_path, "w+");
					if (p_record_file != NULL)
					{
						fwrite(chat_talk_handle->status.record_data, chat_talk_handle->status.record_len, 1, p_record_file);
						fclose(p_record_file);
					}
#endif
					int start_time = get_time_of_day();
					if (baidu_asr(&chat_talk_handle->baidu_acount, chat_talk_handle->asr_result.asr_result, sizeof(chat_talk_handle->asr_result.asr_result), 
						(unsigned char*)chat_talk_handle->status.record_data, chat_talk_handle->status.record_len) == BAIDU_ASR_FAIL)
					{
						ESP_LOGE(LOG_TAG, "task_chat_talk baidu_asr first fail");
						if (baidu_asr(&chat_talk_handle->baidu_acount, chat_talk_handle->asr_result.asr_result, sizeof(chat_talk_handle->asr_result.asr_result), 
							(unsigned char*)chat_talk_handle->status.record_data, chat_talk_handle->status.record_len) == BAIDU_ASR_FAIL)
						{
							ESP_LOGE(LOG_TAG, "task_chat_talk baidu_asr second fail");
						}
					}

					int asr_cost = get_time_of_day() - start_time;

					if (strlen(chat_talk_handle->asr_result.asr_result) > 0)
					{
						nlp_service_send_request(chat_talk_handle->asr_result.asr_result, chat_talk_nlp_result_cb);
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_2_DYY_NOT_HEAR_CLEARLY_PLEASE_REPEAT);
					}
					
					ESP_LOGI(LOG_TAG, "baidu asr:record_len[%d],cost[%dms],result[%s]", chat_talk_handle->status.record_len, asr_cost, chat_talk_handle->asr_result.asr_result);
					break;
				}
				default:
					break;
			}

			if (msg.msg_type == CHAT_TALK_MSG_QUIT)
			{
				break;
			}

			if (msg.p_record_data != NULL)
			{
				esp32_free(msg.p_record_data);
				msg.p_record_data = NULL;
			}
		}
    }

	Encoder_Interface_exit(chat_talk_handle->amr_encode_handle);
    vTaskDelete(NULL);
}

static void set_chat_talk_record_data(
	int id, char *data, size_t data_len, int record_ms, int is_max)
{
	CHAT_TALK_MSG_T msg = {0};

	if (g_chat_talk_handle == NULL || g_chat_talk_handle->msg_queue == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_chat_talk_record_data invalid parmas");
		return;
	}

	msg.p_record_data = (CHAT_TALK_RECORD_DATA_T *)esp32_malloc(sizeof(CHAT_TALK_RECORD_DATA_T));
	if (msg.p_record_data == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_chat_talk_record_data esp32_malloc failed");
		return;
	}

	memset(msg.p_record_data, 0, sizeof(CHAT_TALK_RECORD_DATA_T));
	if (id == 1)
	{
		msg.msg_type = CHAT_TALK_MSG_RECORD_START;
	}
	else if (id > 1)
	{
		msg.msg_type = CHAT_TALK_MSG_RECORD_READ;
	}
	else
	{
		msg.msg_type = CHAT_TALK_MSG_RECORD_STOP;	
	}
	
	msg.p_record_data->record_id = id;
	msg.p_record_data->record_len = data_len;
	msg.p_record_data->record_ms = record_ms;
	msg.p_record_data->is_max_ms = is_max;
	
	if (data_len > 0)
	{
		memcpy(msg.p_record_data->data, data, data_len);
	}

	if (xQueueSend(g_chat_talk_handle->msg_queue, &msg, 0) != pdPASS)
	{
		ESP_LOGE(LOG_TAG, "xQueueSend g_chat_talk_handle->msg_queue failed");
		esp32_free(msg.p_record_data);
		msg.p_record_data = NULL;
	}
}

void chat_talk_create(MediaService *self)
{
	g_chat_talk_handle = (CHAT_TALK_HANDLE_T *)esp32_malloc(sizeof(CHAT_TALK_HANDLE_T));
	if (g_chat_talk_handle == NULL)
	{
		return;
	}
	memset(g_chat_talk_handle, 0, sizeof(CHAT_TALK_HANDLE_T));

	g_chat_talk_handle->service = self;
	g_chat_talk_handle->msg_queue = xQueueCreate(20, sizeof(CHAT_TALK_MSG_T));
	g_chat_talk_mux = xSemaphoreCreateMutex();
	
	if (xTaskCreate(task_chat_talk,
					"task_chat_talk",
					1024*8,
					g_chat_talk_handle,
					4,
					NULL) != pdPASS) {

		ESP_LOGE(LOG_TAG, "ERROR creating task_chat_talk task! Out of memory?");
	}
}

void chat_talk_delete(void)
{
	CHAT_TALK_MSG_T msg = {0};
	msg.msg_type = CHAT_TALK_MSG_QUIT;
	
	if (g_chat_talk_handle != NULL && g_chat_talk_handle->msg_queue != NULL)
	{
		xQueueSend(g_chat_talk_handle->msg_queue, &msg, 0);
	}
}

void chat_talk_start(void)
{
	player_mdware_start_record(1, CHAT_TALK_MAX_TIME_MS, set_chat_talk_record_data);
}

void chat_talk_stop(void)
{
	player_mdware_stop_record();
}

