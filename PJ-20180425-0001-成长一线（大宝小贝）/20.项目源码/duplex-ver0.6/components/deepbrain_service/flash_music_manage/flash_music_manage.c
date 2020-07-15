#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

static int fls_music_index = TONE_TYPE_BT_RECONNECT;

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
	if (nvs_set_i32(my_handle, FLASH_MUSIC_NAME, fls_music_index) != ESP_OK) 
	{
		if (nvs_set_i32(my_handle, FLASH_MUSIC_NAME, fls_music_index) != ESP_OK)
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
	if (nvs_get_i32(my_handle, FLASH_MUSIC_NAME, &fls_music_index) != ESP_OK)
	{
		err = nvs_get_i32(my_handle, FLASH_MUSIC_NAME, &fls_music_index);
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

void play_current_flash_music()
{
	get_flash_music_index();
	EspAudioPlay(tone_uri[fls_music_index]);
}

void play_prev_flash_music()
{
	fls_music_index--;
	if (fls_music_index < TONE_TYPE_BT_RECONNECT)
	{
		fls_music_index = TONE_TYPE_WELCOME_TO_WIFI;
	}
	EspAudioPlay(tone_uri[fls_music_index]);
	save_flash_music_index();
}

void play_next_flash_music()
{
	fls_music_index++;
	if (fls_music_index > TONE_TYPE_WELCOME_TO_WIFI)
	{
		fls_music_index = TONE_TYPE_BT_RECONNECT;
	}
	EspAudioPlay(tone_uri[fls_music_index]);
	save_flash_music_index();
}