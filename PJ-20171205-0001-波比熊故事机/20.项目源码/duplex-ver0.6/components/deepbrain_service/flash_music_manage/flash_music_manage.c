#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "freertos/queue.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "EspAudio.h"
#include "AudioDef.h"
#include "toneuri.h"
#include "flash_music_manage.h"
#include "flash_config_manage.h"
#include "auto_play_service.h"
//#include "player_middleware.h"

static ToneType g_fls_music_url_index = TONE_TYPE_STORY_01;

esp_err_t save_flash_music_index()
{
	nvs_handle my_handle;
	esp_err_t err;
	
	err = nvs_open(DEVICE_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) 
	{
		ESP_LOGE(FLASH_MUSIC_TAG, "save_flash_music_index nvs_open fail");
		nvs_close(my_handle);
		return ESP_FAIL;
	}
	if (nvs_set_i32(my_handle, FLASH_MUSIC_NAME, g_fls_music_url_index) != ESP_OK) 
	{
		if (nvs_set_i32(my_handle, FLASH_MUSIC_NAME, g_fls_music_url_index) != ESP_OK)
		{
			ESP_LOGE(FLASH_MUSIC_TAG, "save_flash_music_index nvs_set_blob %s fail", FLASH_MUSIC_TAG);
			nvs_close(my_handle);
			return ESP_FAIL;
		}
	}
	
	if (nvs_commit(my_handle) != ESP_OK) 
	{
		ESP_LOGE(FLASH_MUSIC_TAG, "save_flash_music_index nvs_commit fail");
		nvs_close(my_handle);
		return ESP_FAIL;
	}
	nvs_close(my_handle);
	ESP_LOGE(FLASH_MUSIC_TAG, "save_flash_music_index success!");
	return ESP_OK;
}

esp_err_t get_flash_music_index()
{
	nvs_handle my_handle;
	esp_err_t err;
	
	err = nvs_open(DEVICE_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) 
	{
		ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "get_infos nvs_open fail");
		nvs_close(my_handle);
		printf("in get_flash_music_index return err1! err = %d\n", err);
		return ESP_FAIL;
	}
	if (nvs_get_i32(my_handle, FLASH_MUSIC_NAME, &g_fls_music_url_index) != ESP_OK)
	{
		err = nvs_get_i32(my_handle, FLASH_MUSIC_NAME, &g_fls_music_url_index);
		if (err != ESP_OK)
		{
			printf("in get_flash_music_index return err2! err = %d\n", err);
			nvs_close(my_handle);
			return ESP_FAIL;
		}
	}
	nvs_close(my_handle);
	return ESP_OK;
}

static void get_prev_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	g_fls_music_url_index--;
	if (g_fls_music_url_index < TONE_TYPE_STORY_01 || g_fls_music_url_index > TONE_TYPE_STORY_19)
	{
		g_fls_music_url_index = TONE_TYPE_STORY_19;
	}
	snprintf(src->music_url, sizeof(src->music_url), "%s", tone_uri[g_fls_music_url_index]);
	save_flash_music_index();
}

static void get_next_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	g_fls_music_url_index++;
	if (g_fls_music_url_index > TONE_TYPE_STORY_19 || g_fls_music_url_index < TONE_TYPE_STORY_01)
	{
		g_fls_music_url_index = TONE_TYPE_STORY_01;
	}
	snprintf(src->music_url, sizeof(src->music_url), "%s", tone_uri[g_fls_music_url_index]);
	save_flash_music_index();
}

static void get_current_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{	
	if (src == NULL)
	{
		return;
	}
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	get_flash_music_index();
	//待完善，可以get 到src->music_url
	snprintf(src->music_url, sizeof(src->music_url), "%s", tone_uri[g_fls_music_url_index]);
}

void flash_music_start()
{
	AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_music, get_next_music, get_current_music};
	
	set_auto_play_cb(&call_backs);
}