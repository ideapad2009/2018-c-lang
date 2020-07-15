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
#include "TouchControlService.h"
#include "EspAudio.h"
#include "functional_running_state_check.h"
#include "guo_xue_function_manage.h"
#include "auto_play_service.h"
#include "esp_log.h"

#define GUEXUE_STORAGE_NAME		"keyword_key"
#define GUO_XUE_LOG_TAG			"guo_xue_function"
#define RETURN_NULL				1

static GUO_XUE_HANDLE *g_guo_xue_handle = NULL;
char *g_guo_xue_keyword = NULL;

void guo_xue_result_cb(NLP_RESULT_T *result);

//保存目录关键词到flash中
esp_err_t set_guoxue_keyword(char *guoxue_keyword)
{
	printf("in set_guoxue_keyword!\n");
	nvs_handle my_handle;
    esp_err_t err;
    err = nvs_open(DEVICE_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
	{
		nvs_close(my_handle);
		return ESP_FAIL;
	}
    err = nvs_set_str(my_handle, GUEXUE_STORAGE_NAME, guoxue_keyword);
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
esp_err_t get_guoxue_keyword()
{
	nvs_handle my_handle;
    esp_err_t err;
	
	printf("in get_and_play_guoxue_keyword!\n");

	err = nvs_open(DEVICE_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
	{
		printf("nvs_open faied!\n");
		nvs_close(my_handle);
		return ESP_FAIL;
    }
	size_t required_size = 12;
    err = nvs_get_str(my_handle, GUEXUE_STORAGE_NAME, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
	{
		printf("nvs_get_str 1 faied!\n");
		nvs_close(my_handle);
		return ESP_FAIL;
    }
    if (required_size == 0)
	{
		nvs_close(my_handle);
		return RETURN_NULL;
    }
	else
	{
        err = nvs_get_str(my_handle, GUEXUE_STORAGE_NAME, g_guo_xue_keyword, &required_size);
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

//判断使用国学分类前的分类，对比关键词
void switch_guoxue_keyword_type()
{
	printf("in switch_guoxue_keyword_type!\n");
	
	if (strcmp(g_guo_xue_keyword, "百家姓") == 0)
	{
		nlp_service_send_request("千字文", guo_xue_result_cb);
		set_guoxue_keyword("千字文");
	}
	else if (strcmp(g_guo_xue_keyword, "千字文") == 0)
	{
		nlp_service_send_request("弟子规", guo_xue_result_cb);
		set_guoxue_keyword("弟子规");
	}
	else if (strcmp(g_guo_xue_keyword, "弟子规") == 0)
	{
		nlp_service_send_request("论语", guo_xue_result_cb);
		set_guoxue_keyword("论语");
	}
	else if (strcmp(g_guo_xue_keyword, "论语") == 0)
	{
		nlp_service_send_request("三字经", guo_xue_result_cb);
		set_guoxue_keyword("三字经");
	}
	else if (strcmp(g_guo_xue_keyword, "三字经") == 0)
	{
		nlp_service_send_request("百家姓", guo_xue_result_cb);
		set_guoxue_keyword("百家姓");
	}
	else
	{	
		nlp_service_send_request("百家姓", guo_xue_result_cb);
		set_guoxue_keyword("百家姓");
	}
}

static void get_prev_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_guo_xue_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_guo_xue_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	if (g_guo_xue_handle->cur_play_index > 0)
	{
		g_guo_xue_handle->cur_play_index--;
	}

	ESP_LOGI(GUO_XUE_LOG_TAG, "get_prev_music [%d][%s][%s]", 
		g_guo_xue_handle->cur_play_index+1,
		p_links->link[g_guo_xue_handle->cur_play_index].link_name,
		p_links->link[g_guo_xue_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_guo_xue_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_guo_xue_handle->cur_play_index].link_url);
}


static void get_next_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_guo_xue_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_guo_xue_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	if (g_guo_xue_handle->cur_play_index < p_links->link_size - 1)
	{
		g_guo_xue_handle->cur_play_index++;
	}
	else if (g_guo_xue_handle->cur_play_index == p_links->link_size - 1)
	{
		if (strlen(g_guo_xue_handle->nlp_result.input_text) > 0 && p_links->link_size == NLP_SERVICE_LINK_MAX_NUM)
		{
			nlp_service_send_request(g_guo_xue_handle->nlp_result.input_text, guo_xue_result_cb);
		}
		return;
	}

	ESP_LOGI(GUO_XUE_LOG_TAG, "get_next_music [%d][%s][%s]", 
		g_guo_xue_handle->cur_play_index+1,
		p_links->link[g_guo_xue_handle->cur_play_index].link_name,
		p_links->link[g_guo_xue_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_guo_xue_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_guo_xue_handle->cur_play_index].link_url);
}

static void get_current_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_guo_xue_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_guo_xue_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	ESP_LOGI(GUO_XUE_LOG_TAG, "get_current_music [%d][%s][%s]", 
		g_guo_xue_handle->cur_play_index+1,
		p_links->link[g_guo_xue_handle->cur_play_index].link_name,
		p_links->link[g_guo_xue_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_guo_xue_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_guo_xue_handle->cur_play_index].link_url);
}

void guo_xue_result_cb(NLP_RESULT_T *result)
{
	NLP_RESULT_LINKS_T *p_links = &result->link_result;
	AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_music, get_next_music, get_current_music};

	if (g_guo_xue_handle == NULL)
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
			ESP_LOGE(GUO_XUE_LOG_TAG, "chat_result=[%s]\r\n", result->chat_result.text);
			play_tts(result->chat_result.text);
			break;
		}
		case NLP_RESULT_TYPE_LINK:
		{
			if (p_links->link_size > 0)
			{
				g_guo_xue_handle->nlp_result = *result;
				g_guo_xue_handle->cur_play_index = 0;
				set_auto_play_cb(&call_backs);
			}
			break;
		}
		default:
			break;
	}
}

void guo_xue_play_start()
{
	FUNCTIONAL_TYIGGER_KEY_E key_state = get_functional_tyigger_key_state();
	int ret = get_guoxue_keyword();
	if (key_state == FUNCTIONAL_TYIGGER_KEY_GUOXUE_STATE)
	{
		if (ret != ESP_OK)
		{
			nlp_service_send_request("百家姓", guo_xue_result_cb);
			set_guoxue_keyword("百家姓");
		}
		else
		{
			switch_guoxue_keyword_type(g_guo_xue_keyword);
		}
	}
	else
	{
		nlp_service_send_request(g_guo_xue_keyword, guo_xue_result_cb);
	}
}

void guo_xue_init()
{
	g_guo_xue_handle = malloc(sizeof(GUO_XUE_HANDLE));
	g_guo_xue_keyword = malloc(12);
}

