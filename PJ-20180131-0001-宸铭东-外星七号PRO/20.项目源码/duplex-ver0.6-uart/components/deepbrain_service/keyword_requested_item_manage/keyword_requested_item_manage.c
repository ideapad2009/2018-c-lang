/*用关键字向deepbrain平台请求数据接口*/

#include "auto_play_service.h"
#include "device_api.h"
#include "esp_log.h"
#include "keyword_requested_item_manage.h"
#include "nlp_service.h"
#include "player_middleware.h"
#include <stdio.h>

#define KEYWORD_REQUEST_LOG_TAG 		"keyword_request"

void keyword_request_nlp_result_cb(NLP_RESULT_T *result);

typedef struct
{
	int cur_play_index;
	NLP_RESULT_T nlp_result;
}KEYWARD_REQUESTED_HANDLE_T;

static KEYWARD_REQUESTED_HANDLE_T *g_keyword_request_handle = NULL;

static void get_prev_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_keyword_request_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_keyword_request_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	if (g_keyword_request_handle->cur_play_index > 0)
	{
		g_keyword_request_handle->cur_play_index--;
	}

	ESP_LOGI(KEYWORD_REQUEST_LOG_TAG, "get_prev_music [%d][%s][%s]", 
		g_keyword_request_handle->cur_play_index+1,
		p_links->link[g_keyword_request_handle->cur_play_index].link_name,
		p_links->link[g_keyword_request_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_keyword_request_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_keyword_request_handle->cur_play_index].link_url);
}

static void get_next_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_keyword_request_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_keyword_request_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	if (g_keyword_request_handle->cur_play_index < p_links->link_size - 1)
	{
		g_keyword_request_handle->cur_play_index++;
	}
	else if (g_keyword_request_handle->cur_play_index == p_links->link_size - 1)
	{
		if (strlen(g_keyword_request_handle->nlp_result.input_text) > 0 && p_links->link_size == NLP_SERVICE_LINK_MAX_NUM)
		{
			nlp_service_send_request(g_keyword_request_handle->nlp_result.input_text, keyword_request_nlp_result_cb);
		}
		return;
	}

	ESP_LOGI(KEYWORD_REQUEST_LOG_TAG, "get_next_music [%d][%s][%s]", 
		g_keyword_request_handle->cur_play_index+1,
		p_links->link[g_keyword_request_handle->cur_play_index].link_name,
		p_links->link[g_keyword_request_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_keyword_request_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_keyword_request_handle->cur_play_index].link_url);
}

static void get_current_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_keyword_request_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_keyword_request_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	ESP_LOGI(KEYWORD_REQUEST_LOG_TAG, "get_current_music [%d][%s][%s]", 
		g_keyword_request_handle->cur_play_index+1,
		p_links->link[g_keyword_request_handle->cur_play_index].link_name,
		p_links->link[g_keyword_request_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_keyword_request_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_keyword_request_handle->cur_play_index].link_url);
}

void keyword_request_nlp_result_cb(NLP_RESULT_T *result)
{
	NLP_RESULT_LINKS_T *p_links = &result->link_result;
	AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_music, get_next_music, get_current_music};

	if (g_keyword_request_handle == NULL)
	{
		return;
	}

	switch (result->type)
	{
		case NLP_RESULT_TYPE_LINK:
		{						
			if (p_links->link_size > 0)
			{
				g_keyword_request_handle->nlp_result = *result;
				g_keyword_request_handle->cur_play_index = 0;
				set_auto_play_cb(&call_backs);
			}
			else
			{
				player_mdware_play_tone(FLASH_MUSIC_NO_MUSIC);
			}
			break;
		}
		case NLP_RESULT_TYPE_CHAT:
		{
			
			play_tts(result->chat_result.text);
			break;
		}
		default:
		{
			player_mdware_play_tone(FLASH_MUSIC_NO_MUSIC);
			break;
		}
	}
}

void keyword_request_nlp_start(const char *keyword)
{
	nlp_service_send_request(keyword, keyword_request_nlp_result_cb);
}

void init_keyword_request_nlp()
{
	g_keyword_request_handle = esp32_malloc(sizeof(KEYWARD_REQUESTED_HANDLE_T));
	if (g_keyword_request_handle == NULL)
	{
		ESP_LOGE(KEYWORD_REQUEST_LOG_TAG, "init_keyword_request_nlp failed, out of memory");
		return;
	}
	memset((char*)g_keyword_request_handle, 0, sizeof(KEYWARD_REQUESTED_HANDLE_T));
}

void news_request_start()
{
	keyword_request_nlp_start(KEYWORD_NEWS);
}
