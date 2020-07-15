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
#include "speex.h"
#include "amrwb_encode.h"
#include "user_experience_test_data_manage.h"

//#define DEBUG_PRINT
#define LOG_TAG "chat talk"

static CHAT_TALK_HANDLE_T *g_chat_talk_handle = NULL;
static xSemaphoreHandle g_chat_talk_mux = NULL;

extern void chat_talk_nlp_result_cb(NLP_RESULT_T *result);

static void get_prev_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_chat_talk_handle == NULL)
	{
		return;
	}
	xSemaphoreTake(g_chat_talk_mux, portMAX_DELAY);
	NLP_RESULT_LINKS_T *p_links = &g_chat_talk_handle->nlp_result.link_result;
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size > 0)
	{
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
	xSemaphoreGive(g_chat_talk_mux);
}


static void get_next_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_chat_talk_handle == NULL)
	{
		return;
	}

	xSemaphoreTake(g_chat_talk_mux, portMAX_DELAY);
	NLP_RESULT_LINKS_T *p_links = &g_chat_talk_handle->nlp_result.link_result;
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size > 0)
	{
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
			
			xSemaphoreGive(g_chat_talk_mux);
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
	xSemaphoreGive(g_chat_talk_mux);
}

static void get_current_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_chat_talk_handle == NULL)
	{
		return;
	}

	xSemaphoreTake(g_chat_talk_mux, portMAX_DELAY);
	NLP_RESULT_LINKS_T *p_links = &g_chat_talk_handle->nlp_result.link_result;
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size > 0)
	{
		ESP_LOGI(LOG_TAG, "get_current_music [%d][%s][%s]", 
			g_chat_talk_handle->cur_play_index+1,
			p_links->link[g_chat_talk_handle->cur_play_index].link_name,
			p_links->link[g_chat_talk_handle->cur_play_index].link_url);
		
		src->need_play_name = 1;
		snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_chat_talk_handle->cur_play_index].link_name);
		snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_chat_talk_handle->cur_play_index].link_url);
	}
	xSemaphoreGive(g_chat_talk_mux);
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
			ESP_LOGE(LOG_TAG, "p_links->link_size=[%d]\r\n", p_links->link_size);
			if (p_links->link_size > 0)
			{
				g_chat_talk_handle->nlp_result = *result;
				g_chat_talk_handle->cur_play_index = 0;
				set_auto_play_cb(&call_backs);
			}
			break;
		}
		case NLP_RESULT_TYPE_CMD:
		{
			break;
		}
		default:
			break;
	}
}

static void chat_talk_asr_result_cb(ASR_RESULT_T *result)
{
	WECHAT_MSG_T *p_msg = NULL;
	
	if (g_chat_talk_handle == NULL || g_chat_talk_handle->msg_queue == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_wechat_record_data invalid parmas");
		return;
	}
	
	p_msg = (WECHAT_MSG_T *)esp32_malloc(sizeof(WECHAT_MSG_T));
	if (p_msg == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_wechat_record_data esp32_malloc failed");
		return;
	}
	memset(p_msg, 0, sizeof(WECHAT_MSG_T));

	p_msg->msg_type = WECHAT_MSG_TYPE_ASR_RET;
	p_msg->asr_result = *result;
	if (xQueueSend(g_chat_talk_handle->msg_queue, &p_msg, 0) != pdPASS)
	{
		esp32_free(p_msg);
		p_msg = NULL;
	}
	
	return;
}

static void chat_talk_process(WECHAT_MSG_T *p_msg)
{
	int ret = 0;
	
	if (p_msg == NULL)
	{
		return;
	}

	switch (p_msg->asr_result.type)
	{
		case ASR_RESULT_TYPE_SHORT_AUDIO:
		{
			//长按语音键 听到嘟声后 说出你想听的
			player_mdware_play_tone(FLASH_MUSIC_CHAT_SHORT_AUDIO);
			break;
		}
		case ASR_RESULT_TYPE_SUCCESS:
		{
			if (strcmp(p_msg->asr_result.asr_text, "当前版本") == 0)
			{
				play_tts(ESP32_FW_VERSION);
			}
			else
			{
				nlp_service_send_request(p_msg->asr_result.asr_text, chat_talk_nlp_result_cb);
			}
			break;
		}
		case ASR_RESULT_TYPE_FAIL:
		case ASR_RESULT_TYPE_NO_RESULT:
		{
			player_mdware_play_tone(FLASH_MUSIC_2_DYY_NOT_HEAR_CLEARLY_PLEASE_REPEAT);
			break;
		}
		default:
			break;
	}
}


static void task_chat_talk(void *pv)
{
	int ret = 0;
	int i = 0;
	CHAT_TALK_MSG_T *p_msg = NULL;
	CHAT_TALK_HANDLE_T *chat_talk_handle = (CHAT_TALK_HANDLE_T *)pv;
	
	while (1) 
	{
		if (xQueueReceive(chat_talk_handle->msg_queue, &p_msg, portMAX_DELAY) == pdPASS) 
		{			
			switch (p_msg->msg_type)
			{
				case CHAT_TALK_MSG_TYPE_ASR_RET:
				{
					chat_talk_process(p_msg);
					break;
				}
				case CHAT_TALK_MSG_TYPE_QUIT:
				{
					break;
				}
				default:
					break;
			}

			if (p_msg->msg_type == CHAT_TALK_MSG_TYPE_QUIT)
			{
				esp32_free(p_msg);
				p_msg = NULL;
				break;
			}

			if (p_msg != NULL)
			{
				esp32_free(p_msg);
				p_msg = NULL;
			}
		}
	}

	vTaskDelete(NULL);
}

static void set_chat_talk_record_data(
		int id, char *data, size_t data_len, int record_ms, int is_max)
{
	ASR_RECORD_MSG_T *p_msg = NULL;

	if (g_chat_talk_handle == NULL || g_chat_talk_handle->msg_queue == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_wechat_record_data invalid parmas");
		return;
	}

	p_msg = (ASR_RECORD_MSG_T *)esp32_malloc(sizeof(ASR_RECORD_MSG_T));
	if (p_msg == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_wechat_record_data esp32_malloc failed");
		return;
	}

	memset(p_msg, 0, sizeof(ASR_RECORD_MSG_T));
	if (id == 1)
	{
		p_msg->msg_type = ASR_SERVICE_RECORD_START;
	}
	else if (id > 1)
	{
		p_msg->msg_type = ASR_SERVICE_RECORD_READ;
	}
	else
	{
		p_msg->msg_type = ASR_SERVICE_RECORD_STOP;
		player_mdware_play_tone(FLASH_MUSIC_MSG_SEND);
	}
	
	p_msg->record_data.record_id = id;
	p_msg->record_data.record_len = data_len;
	p_msg->record_data.record_ms = record_ms;
	p_msg->record_data.is_max_ms = is_max;
	p_msg->record_data.time_stamp = get_time_of_day();
	get_flash_cfg(FLASH_CFG_ASR_MODE, &p_msg->record_data.record_type);
	p_msg->call_back = chat_talk_asr_result_cb;
	
	if (data_len > 0)
	{
		memcpy(p_msg->record_data.record_data, data, data_len);
	}

	if (asr_service_send_request(p_msg) == ESP_FAIL)
	{
		ESP_LOGE(LOG_TAG, "set_wechat_record_data asr_service_send_request failed");
		esp32_free(p_msg);
		p_msg = NULL;
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
	g_chat_talk_handle->msg_queue = xQueueCreate(2, sizeof(char*));
	g_chat_talk_mux = xSemaphoreCreateMutex();
	
	if (xTaskCreate(task_chat_talk,
					"task_chat_talk",
					1024*3,
					g_chat_talk_handle,
					4,
					NULL) != pdPASS) {

		ESP_LOGE(LOG_TAG, "ERROR creating task_chat_talk task! Out of memory?");
	}
}

void chat_talk_delete(void)
{

}

void chat_talk_start(void)
{
	player_mdware_start_record(1, ASR_AMR_MAX_TIME_MS, set_chat_talk_record_data);
}

void chat_talk_stop(void)
{
	player_mdware_stop_record(set_chat_talk_record_data);
}

