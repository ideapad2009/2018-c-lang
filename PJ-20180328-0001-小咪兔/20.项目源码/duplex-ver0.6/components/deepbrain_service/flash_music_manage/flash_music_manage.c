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
#include "auto_play_service.h"
#include "flash_music_manage.h"
#include "flash_config_manage.h"

#define PRINT_TAG	"flash music manage"

static void get_prev_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	int ret = 0;
	int i_flash_index = -1;

	if (src != NULL)
	{
		memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	}
	else
	{
		return;
	}

	ret = get_flash_cfg(FLASH_CFG_FLASH_MUSIC_INDEX, &i_flash_index);
	if (ret != ESP_OK)
	{
		ESP_LOGE(PRINT_TAG, "FLASH_CFG_INTERNEL_MUSIC_INDEX get failed! i_flash_index get ret= %d, ESP_ERR_NVS_BASE = %d\n", 
			ret, ESP_ERR_NVS_BASE);
		i_flash_index = TONE_TYPE_STORY_01;
	}
	else
	{
		ESP_LOGI(PRINT_TAG, "i_flash_index = %d\n", i_flash_index);
		if (i_flash_index < TONE_TYPE_STORY_01 || i_flash_index > TONE_TYPE_STORY_10)
		{
			i_flash_index = TONE_TYPE_STORY_01;
		}
	}

	if (i_flash_index == TONE_TYPE_STORY_01)
	{
		i_flash_index = TONE_TYPE_STORY_10;
	}
	else
	{
		i_flash_index--;
	}
	
	snprintf(src->music_url, sizeof(src->music_url), "%s", tone_uri[i_flash_index]);
	set_flash_cfg(FLASH_CFG_FLASH_MUSIC_INDEX, &i_flash_index);
}

static void get_next_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	int ret = 0;
	int i_flash_index = -1;

	if (src != NULL)
	{
		memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	}
	else
	{
		return;
	}

	ret = get_flash_cfg(FLASH_CFG_FLASH_MUSIC_INDEX, &i_flash_index);
	if (ret != ESP_OK)
	{
		ESP_LOGE(PRINT_TAG, "FLASH_CFG_INTERNEL_MUSIC_INDEX get failed! i_flash_index get ret= %d, ESP_ERR_NVS_BASE = %d\n", 
			ret, ESP_ERR_NVS_BASE);
		i_flash_index = TONE_TYPE_STORY_01;
	}
	else
	{
		ESP_LOGI(PRINT_TAG, "i_flash_index = %d\n", i_flash_index);
		if (i_flash_index < TONE_TYPE_STORY_01 || i_flash_index > TONE_TYPE_STORY_10)
		{
			i_flash_index = TONE_TYPE_STORY_01;
		}
	}

	if (i_flash_index == TONE_TYPE_STORY_10)
	{
		i_flash_index = TONE_TYPE_STORY_01;
	}
	else
	{
		i_flash_index++;
	}
	
	snprintf(src->music_url, sizeof(src->music_url), "%s", tone_uri[i_flash_index]);
	set_flash_cfg(FLASH_CFG_FLASH_MUSIC_INDEX, &i_flash_index);
}

static void get_current_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	int ret = 0;
	int i_flash_index = -1;

	if (src != NULL)
	{
		memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	}
	else
	{
		return;
	}

	ret = get_flash_cfg(FLASH_CFG_FLASH_MUSIC_INDEX, &i_flash_index);
	if (ret != ESP_OK)
	{
		ESP_LOGE(PRINT_TAG, "FLASH_CFG_INTERNEL_MUSIC_INDEX get failed! i_flash_index get ret= %d, ESP_ERR_NVS_BASE = %d\n", 
			ret, ESP_ERR_NVS_BASE);
		i_flash_index = TONE_TYPE_STORY_01;
	}
	else
	{
		ESP_LOGI(PRINT_TAG, "i_flash_index = %d\n", i_flash_index);
		if (i_flash_index < TONE_TYPE_STORY_01 || i_flash_index > TONE_TYPE_STORY_10)
		{
			i_flash_index = TONE_TYPE_STORY_01;
		}
	}
	
	snprintf(src->music_url, sizeof(src->music_url), "%s", tone_uri[i_flash_index]);
	set_flash_cfg(FLASH_CFG_FLASH_MUSIC_INDEX, &i_flash_index);
}

void flash_music_start()
{
	AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_music, get_next_music, get_current_music};

	set_auto_play_cb(&call_backs);
}