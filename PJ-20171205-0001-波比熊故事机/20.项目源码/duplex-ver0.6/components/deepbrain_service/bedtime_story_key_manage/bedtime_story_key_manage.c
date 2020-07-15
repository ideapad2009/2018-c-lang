#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "flash_config_manage.h"
#include "DeviceCommon.h"
#include "EspAudio.h"
#include "functional_running_state_check.h"
#include "bedtime_story_key_manage.h"
#include "auto_play_service.h"
#include "esp_log.h"

#define BEDTIME_STORAGE_NAME			"bedtime_story"
#define _RETURN_NULL					1
#define BEDTIME_STORY_LOG_TAG			"bedtime_story_function"
#define KEY_WORD_STORY					"睡前故事"	
#define KEY_WORD_MUSIC					"睡前音乐"

static BEDTIME_STORY_HANDLE *g_bedtime_story_handle = NULL;
char *g_bedtime_story_keyword = NULL;

void bedtime_story_result_cb(NLP_RESULT_T *result);

esp_err_t set_bedtime_story_keyword(char *bedtime_story_keyword)
{
	printf("in set_bedtime_story_keyword!\n");
	nvs_handle my_handle;
    esp_err_t err;
    err = nvs_open(DEVICE_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
	{
		nvs_close(my_handle);
		return ESP_FAIL;
	}
    err = nvs_set_str(my_handle, BEDTIME_STORAGE_NAME, bedtime_story_keyword);
    if (err != ESP_OK)
	{
		nvs_close(my_handle);
		return ESP_FAIL;
	}
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
	{
		nvs_close(my_handle);
		return ESP_FAIL;
    }
	
    nvs_close(my_handle);
	
	return ESP_OK;
}

//获取保存在flash中的目录关键词
esp_err_t get_bedtime_story_keyword()
{
	nvs_handle my_handle;
    esp_err_t err;
	
	printf("in get_bedtime_story_keyword!\n");

	err = nvs_open(DEVICE_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
	{
		printf("nvs_open faied!\n");
		nvs_close(my_handle);
		return ESP_FAIL;
    }
	size_t required_size = 12;
    err = nvs_get_str(my_handle, BEDTIME_STORAGE_NAME, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
	{
		printf("nvs_get_str 1 faied!\n");
		nvs_close(my_handle);
		return ESP_FAIL;
    }
    if (required_size == 0)
	{
		nvs_close(my_handle);
		return _RETURN_NULL;
    }
	else
	{
        err = nvs_get_str(my_handle, BEDTIME_STORAGE_NAME, g_bedtime_story_keyword, &required_size);
        if (err != ESP_OK)
		{
			printf("nvs_get_str 2 faied!\n");
			nvs_close(my_handle);
			return ESP_FAIL;
        }
    }
    nvs_close(my_handle);

	return ESP_OK;
}

static void get_prev_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_bedtime_story_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_bedtime_story_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	if (g_bedtime_story_handle->cur_play_index > 0)
	{
		g_bedtime_story_handle->cur_play_index--;
	}

	ESP_LOGI(BEDTIME_STORY_LOG_TAG, "get_prev_music [%d][%s][%s]", 
		g_bedtime_story_handle->cur_play_index+1,
		p_links->link[g_bedtime_story_handle->cur_play_index].link_name,
		p_links->link[g_bedtime_story_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_bedtime_story_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_bedtime_story_handle->cur_play_index].link_url);
}


static void get_next_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_bedtime_story_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_bedtime_story_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	if (g_bedtime_story_handle->cur_play_index < p_links->link_size - 1)
	{
		g_bedtime_story_handle->cur_play_index++;
	}
	else if (g_bedtime_story_handle->cur_play_index == p_links->link_size - 1)
	{
		if (strlen(g_bedtime_story_handle->nlp_result.input_text) > 0 && p_links->link_size == NLP_SERVICE_LINK_MAX_NUM)
		{
			nlp_service_send_request(g_bedtime_story_handle->nlp_result.input_text, bedtime_story_result_cb);
		}
		return;
	}

	ESP_LOGI(BEDTIME_STORY_LOG_TAG, "get_next_music [%d][%s][%s]", 
		g_bedtime_story_handle->cur_play_index+1,
		p_links->link[g_bedtime_story_handle->cur_play_index].link_name,
		p_links->link[g_bedtime_story_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_bedtime_story_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_bedtime_story_handle->cur_play_index].link_url);
}

static void get_current_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_bedtime_story_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_bedtime_story_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	ESP_LOGI(BEDTIME_STORY_LOG_TAG, "get_current_music [%d][%s][%s]", 
		g_bedtime_story_handle->cur_play_index+1,
		p_links->link[g_bedtime_story_handle->cur_play_index].link_name,
		p_links->link[g_bedtime_story_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_bedtime_story_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_bedtime_story_handle->cur_play_index].link_url);
}

void bedtime_story_result_cb(NLP_RESULT_T *result)
{
	NLP_RESULT_LINKS_T *p_links = &result->link_result;
	AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_music, get_next_music, get_current_music};

	if (g_bedtime_story_handle == NULL)
	{
		return;
	}
	
	switch (result->type)
	{
		case NLP_RESULT_TYPE_NONE:
		{
			break;
		}
		case NLP_RESULT_TYPE_CHAT:
		{
			//play tts
			auto_play_pause();
			ESP_LOGE(BEDTIME_STORY_LOG_TAG, "chat_result=[%s]\r\n", result->chat_result.text);
			play_tts(result->chat_result.text);
			break;
		}
		case NLP_RESULT_TYPE_LINK:
		{
			if (p_links->link_size > 0)
			{
				g_bedtime_story_handle->nlp_result = *result;
				g_bedtime_story_handle->cur_play_index = 0;
				set_auto_play_cb(&call_backs);
			}
			break;
		}
		default:
			break;
	}
}

//判断使用国学分类前的分类，对比关键词
void switch_bedtime_story_keyword_type()
{
	printf("in switch_bedtime_story_keyword_type!\n");
	
	if (strcmp(g_bedtime_story_keyword, KEY_WORD_STORY) == 0)
	{
		nlp_service_send_request(KEY_WORD_MUSIC, bedtime_story_result_cb);
		set_bedtime_story_keyword(KEY_WORD_MUSIC);
	}
	else if (strcmp(g_bedtime_story_keyword, KEY_WORD_MUSIC) == 0)
	{
		nlp_service_send_request(KEY_WORD_STORY, bedtime_story_result_cb);
		set_bedtime_story_keyword(KEY_WORD_STORY);
	}
	else
	{	
		nlp_service_send_request(KEY_WORD_STORY, bedtime_story_result_cb);
		set_bedtime_story_keyword(KEY_WORD_STORY);
	}
}

void bedtime_story_play_start()
{	
	FUNCTIONAL_TYIGGER_KEY_E key_state = get_functional_tyigger_key_state();
	int ret = get_bedtime_story_keyword();
	if (key_state == FUNCTIONAL_TYIGGER_KEY_BEDTIME_STORY_STATE)
	{
		if (ret != ESP_OK)
		{
			nlp_service_send_request(KEY_WORD_STORY, bedtime_story_result_cb);
			set_bedtime_story_keyword(KEY_WORD_STORY);
		}
		else
		{
			switch_bedtime_story_keyword_type(g_bedtime_story_keyword);
		}
	}
	else
	{
		nlp_service_send_request(g_bedtime_story_keyword, bedtime_story_result_cb);
	}
}

void bedtime_story_function_init()
{
	g_bedtime_story_handle = malloc(sizeof(BEDTIME_STORY_HANDLE));
	g_bedtime_story_keyword = malloc(12);
}


