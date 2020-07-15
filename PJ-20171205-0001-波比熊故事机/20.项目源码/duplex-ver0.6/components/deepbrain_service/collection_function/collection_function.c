#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "collection_function.h"
#include "flash_config_manage.h"
#include "MediaService.h"
#include "esp_log.h"
#include "EspAudio.h"
#include "AudioDef.h"
#include "toneuri.h"
#include "device_api.h"
#include "TouchControlService.h"
#include "DeviceCommon.h"
#include "auto_play_service.h"
#include "player_middleware.h"

#define URL_SIZE				96	//url大小
#define URL_NULL				11	//url为空

char *g_music_url = NULL;
char *g_music_url_info = NULL;
int g_save_signal = !CAN_SAVE;
int g_get_infos_ret = ESP_FAIL;
SAVE_INDEX_NUM save_index_num = SAVE_INDEX_NUM_ONE;
PLAY_INDEX_NUM latest_play_index_num = PLAY_INDEX_NUM_ONE;

//保存URL
esp_err_t save_url()
{
	nvs_handle my_handle;
    esp_err_t err;
	char save_index_text[2];
	
	itoa(save_index_num, save_index_text, 10);
    err = nvs_open(COLLECTION_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
	{
		nvs_close(my_handle);
		return ESP_FAIL;
	}
    err = nvs_set_str(my_handle, save_index_text, g_music_url);
    if (err != ESP_OK)
	{
		err = nvs_set_str(my_handle, save_index_text, g_music_url);
		if (err != ESP_OK)
		{
			nvs_close(my_handle);
			return ESP_FAIL;
		}
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

esp_err_t save_save_url_index()
{
	nvs_handle my_handle;
    esp_err_t err;
	
    err = nvs_open(COLLECTION_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
	{
		nvs_close(my_handle);
		return ESP_FAIL;
	}
    err = nvs_set_i32(my_handle, SAVE_URL_INDEX_NAMESPACE, save_index_num);
    if (err != ESP_OK)
	{
		err = nvs_set_i32(my_handle, SAVE_URL_INDEX_NAMESPACE, save_index_num);
		if (err != ESP_OK)
		{
			nvs_close(my_handle);
			return ESP_FAIL;
		}
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

esp_err_t save_play_url_index()
{
	nvs_handle my_handle;
    esp_err_t err;
	
    err = nvs_open(COLLECTION_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
	{
		nvs_close(my_handle);
		return ESP_FAIL;
	}
    err = nvs_set_i32(my_handle, PLAY_URL_INDEX_NAMESPACE, latest_play_index_num);
    if (err != ESP_OK)
	{
		err = nvs_set_i32(my_handle, PLAY_URL_INDEX_NAMESPACE, latest_play_index_num);
		if (err != ESP_OK)
		{
			nvs_close(my_handle);
			return ESP_FAIL;
		}
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

void set_get_infos_ret(int get_infos_ret)
{
	g_get_infos_ret = get_infos_ret;
}

esp_err_t get_get_infos_ret()
{
	return g_get_infos_ret;
}

esp_err_t get_url()
{	
	nvs_handle my_handle;
	esp_err_t err;
	char lastest_play_index_text[2];
	
	itoa(latest_play_index_num, lastest_play_index_text, 10);
	err = nvs_open(COLLECTION_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) 
	{
		ESP_LOGE(COLLECTION_PARAMS_NAMESPACE, "get_infos nvs_open fail");
		nvs_close(my_handle);
		printf("in get_infos return err1! err = %d\n", err);
		return ESP_FAIL;
	}
	size_t required_size = 0;
	err = nvs_get_str(my_handle, lastest_play_index_text, NULL, &required_size);
	//printf("in get_infos 4! lastest_play_index_text is {%s}\n", lastest_play_index_text);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
	{
		nvs_close(my_handle);
		printf("in get_infos return err2! err = %d\n", err);
		return ESP_FAIL;
	}
	if (err == ESP_ERR_NVS_NOT_FOUND)
	{
		nvs_close(my_handle);
		ESP_LOGE(URL_FLASH_STORAGE_NAME, "in[%d]index,flash is NULL!", latest_play_index_num);
		return ESP_ERR_NVS_NOT_FOUND;
	}
	if (required_size > 0)
	{
		size_t len = 96;
		//printf("in get_infos 5! lastest_play_index_text is {%s}\n", lastest_play_index_text);
		err = nvs_get_str(my_handle, lastest_play_index_text, g_music_url_info, &len);
		if (err != ESP_OK) 
		{
			err = nvs_get_str(my_handle, lastest_play_index_text, g_music_url_info, &len);
			if (err != ESP_OK)
			{
				ESP_LOGE(URL_FLASH_STORAGE_NAME, "in get_url nvs_get_str %s fail", URL_FLASH_STORAGE_NAME);
				nvs_close(my_handle);
				printf("in get_infos return err3! err = %d\n", err);
				return ESP_FAIL;
			}
		}
	}
	else
	{
		nvs_close(my_handle);
		ESP_LOGE(URL_FLASH_STORAGE_NAME, "in[%d]index,flash is NULL!", latest_play_index_num);
		return ESP_ERR_NVS_NOT_FOUND;	//get到的信息不正确，当作空处理
	}
	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t get_save_index_num()
{	
	nvs_handle my_handle;
	esp_err_t err;
	
	err = nvs_open(COLLECTION_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) 
	{
		ESP_LOGE(COLLECTION_PARAMS_NAMESPACE, "get_save_index_num nvs_open fail");
		nvs_close(my_handle);
		printf("in get_save_index_num return err1! err = %d\n", err);
		return ESP_FAIL;
	}
	size_t required_size = 0;
	err = nvs_get_i32(my_handle, SAVE_URL_INDEX_NAMESPACE, &save_index_num);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
	{
		err = nvs_get_i32(my_handle, SAVE_URL_INDEX_NAMESPACE, &save_index_num);
		if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
		{
			printf("in get_save_index_num return err2! err = %d\n", err);
			return ESP_FAIL;
		}
	}
	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t get_play_index_num()
{	
	nvs_handle my_handle;
	esp_err_t err;
	
	err = nvs_open(COLLECTION_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) 
	{
		ESP_LOGE(COLLECTION_PARAMS_NAMESPACE, "get_play_index_num nvs_open fail");
		nvs_close(my_handle);
		printf("in get_play_index_num return err1! err = %d\n", err);
		return ESP_FAIL;
	}
	size_t required_size = 0;
	err = nvs_get_i32(my_handle, PLAY_URL_INDEX_NAMESPACE, &latest_play_index_num);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
	{
		err = nvs_get_i32(my_handle, PLAY_URL_INDEX_NAMESPACE, &latest_play_index_num);
		if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
		{
			printf("in get_play_index_num return err2! err = %d\n", err);
			return ESP_FAIL;
		}
	}
	nvs_close(my_handle);
	return ESP_OK;
}
esp_err_t get_collection_infos()
{
	if (get_save_index_num() != ESP_OK)
	{
		printf("in get_collection_infos, get_save_index_num FAIL!\n");
		return ESP_FAIL;
	}
	if (get_play_index_num() != ESP_OK)
	{
		printf("in get_collection_infos, get_play_index_num FAIL!\n");
		return ESP_FAIL;
	}
	int ret = get_url();
	if (ret == ESP_FAIL)
	{
		printf("in get_collection_infos, get_url FAIL!\n");
		return ESP_FAIL;
	}
	else if (ret == ESP_ERR_NVS_NOT_FOUND)
	{
		printf("in get_collection_infos, NOT FOUND url!\n");
		return ESP_ERR_NVS_NOT_FOUND;
	}
	//printf("get_collection_infos OK!\n");
	return ESP_OK;
}

//排除重复
EXCLUDE_RET_E exclude_repetition_url()
{
	int ret = ESP_FAIL;
	EXCLUDE_RET_E exclude_ret = EXCLUDE_RET_INITIAL;
	PLAY_INDEX_NUM _latest_play_index_num = latest_play_index_num;
	latest_play_index_num = PLAY_INDEX_NUM_ONE;
	
	while (exclude_ret == EXCLUDE_RET_INITIAL)
	{
		if (latest_play_index_num <= PLAY_INDEX_NUM_TWENTY)
		{
			ret = get_url();
			if (ret == ESP_OK)
			{
				if (strcmp (g_music_url, g_music_url_info) == 0)
				{
					exclude_ret = EXCLUDE_RET_EXIST;
				}
			}
			else if (ret == ESP_FAIL)
			{
				printf ("\nin[%d]index,get faild!\n***************************************************<<get faild>>************************\n", latest_play_index_num);
				ret = get_url();
				while (ret == ESP_FAIL)
				{
					ret = get_url();
				}
			} 
			latest_play_index_num++;
		}
		else
		{
			latest_play_index_num = _latest_play_index_num;
			get_url();
			exclude_ret = EXCLUDE_RET_NOT_EXIST;
		}
	}
	return exclude_ret; 
}

void save_function()
{
	MusicInfo *music_info =malloc(sizeof(MusicInfo));
	if (EspAudioInfoGet(music_info) != ESP_OK || music_info->name == NULL)
	{
		free(music_info);
		player_mdware_play_tone(FLASH_MUSIC_B_03_CAN_NOT_BE_COLLECTED_AT_PRESENT);
//		EspAudioTonePlay(tone_uri[TONE_TYPE_03_CAN_NOT_BE_COLLECTED_AT_PRESENT], TERMINATION_TYPE_NOW);
	}
	else
	{
		snprintf(g_music_url, URL_SIZE, "%s", music_info->name);
		free(music_info);
	    if (strcmp (g_music_url, "") != 0 && strncmp (g_music_url, "http", 4) == 0)
	    {
	    	if (exclude_repetition_url() == EXCLUDE_RET_NOT_EXIST)
	    	{
				if (save_url() == ESP_OK)
			    {
//			    	EspAudioTonePlay(tone_uri[TONE_TYPE_02_ADDED_TO_THE_COLLECTION_LIST], TERMINATION_TYPE_NOW);
					player_mdware_play_tone(FLASH_MUSIC_B_02_ADDED_TO_THE_COLLECTION_LIST);
					++save_index_num;
					if (save_index_num > SAVE_INDEX_NUM_TWENTY)
					{
						save_index_num = SAVE_INDEX_NUM_ONE;
					}
					save_save_url_index();
			    }
			    else
			    {
			    	player_mdware_play_tone(FLASH_MUSIC_B_02_ADDED_TO_THE_COLLECTION_LIST);
//			    	EspAudioTonePlay(tone_uri[TONE_TYPE_17_ADD_TO_THE_COLLECTION_LIST_FAIL], TERMINATION_TYPE_NOW);
			    }
	    	}
			else
			{
				player_mdware_play_tone(FLASH_MUSIC_B_29_COLLECTION_LIST_HAS_ALREADY_HAD_THIS);
//				EspAudioTonePlay(tone_uri[TONE_TYPE_29_COLLECTION_LIST_HAS_ALREADY_HAD_THIS], TERMINATION_TYPE_NOW);
			}
	    }
		else
		{
			player_mdware_play_tone(FLASH_MUSIC_B_03_CAN_NOT_BE_COLLECTED_AT_PRESENT);
//			EspAudioTonePlay(tone_uri[TONE_TYPE_03_CAN_NOT_BE_COLLECTED_AT_PRESENT], TERMINATION_TYPE_NOW);
		}
	}
}

static void get_prev_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));

	//printf("in get_prev_music 1! 000latest_play_index_num = %d\n", latest_play_index_num);
	latest_play_index_num--;
	//printf("000latest_play_index_num = %d\n", latest_play_index_num);
	if (latest_play_index_num < PLAY_INDEX_NUM_ONE)
	{
		latest_play_index_num = PLAY_INDEX_NUM_TWENTY;
		//printf("000latest_play_index_num not change! latest_play_index_num = %d\n", latest_play_index_num);
	}
	if (get_url() == ESP_OK)
	{
		snprintf(src->music_url, sizeof(src->music_url), "%s", g_music_url_info);
		save_play_url_index();
	}
	else
	{
		PLAY_INDEX_NUM _latest_play_index_num = latest_play_index_num;
		while (1)
		{
			latest_play_index_num--;
			//printf("latest_play_index_num = %d\n", latest_play_index_num);
			if (latest_play_index_num < PLAY_INDEX_NUM_ONE)
			{
				latest_play_index_num = PLAY_INDEX_NUM_TWENTY;
				//printf("latest_play_index_num not change! latest_play_index_num = %d\n", latest_play_index_num);
			}
			if (get_url() == ESP_OK)
			{
				snprintf(src->music_url, sizeof(src->music_url), "%s", g_music_url_info);
				save_play_url_index();
				break;
			}
			if (latest_play_index_num == _latest_play_index_num)
			{
				if (get_url() == ESP_OK)
				{
					snprintf(src->music_url, sizeof(src->music_url), "%s", g_music_url_info);
					save_play_url_index();
				}
				else
				{
					vTaskDelay(3000);
					player_mdware_play_tone(FLASH_MUSIC_B_04_THE_COLLECTION_LIST_IS_EMPTY);
//					EspAudioPlay(tone_uri[TONE_TYPE_04_THE_COLLECTION_LIST_IS_EMPTY]);
					vTaskDelay(3000);
				}
				break;
			}
		}
	}
}

static void get_next_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	
	//printf("in get_next_music 1! 000latest_play_index_num = %d\n", latest_play_index_num);
	latest_play_index_num++;
	//printf("000latest_play_index_num = %d\n", latest_play_index_num);
	if (latest_play_index_num > PLAY_INDEX_NUM_TWENTY)
	{
		latest_play_index_num = PLAY_INDEX_NUM_ONE;
		//printf("000latest_play_index_num not change! latest_play_index_num = %d\n", latest_play_index_num);
	}
	if (get_url() == ESP_OK)
	{
		snprintf(src->music_url, sizeof(src->music_url), "%s", g_music_url_info);
		save_play_url_index();
	}
	else
	{
		PLAY_INDEX_NUM _latest_play_index_num = latest_play_index_num;
		while (1)
		{
			latest_play_index_num++;
			//printf("latest_play_index_num = %d\n", latest_play_index_num);
			if (latest_play_index_num > PLAY_INDEX_NUM_TWENTY)
			{
				latest_play_index_num = PLAY_INDEX_NUM_ONE;
				//printf("latest_play_index_num not change! latest_play_index_num = %d\n", latest_play_index_num);
			}
			if (get_url() == ESP_OK)
			{
				snprintf(src->music_url, sizeof(src->music_url), "%s", g_music_url_info);
				save_play_url_index();
				break;
			}
			if (latest_play_index_num == _latest_play_index_num)
			{
				if (get_url() == ESP_OK)
				{
					snprintf(src->music_url, sizeof(src->music_url), "%s", g_music_url_info);
					save_play_url_index();
				}
				else
				{
					vTaskDelay(3000);
					player_mdware_play_tone(FLASH_MUSIC_B_04_THE_COLLECTION_LIST_IS_EMPTY);
//					EspAudioPlay(tone_uri[TONE_TYPE_04_THE_COLLECTION_LIST_IS_EMPTY]);
					vTaskDelay(3000);
				}
				break;
			}
		}
	}
}

static void get_current_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{	
	int get_infos_ret = ESP_FAIL;
	if (src == NULL)
	{
		return;
	}
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	get_infos_ret = get_collection_infos();
	if(get_infos_ret == ESP_OK)
	{
		snprintf(src->music_url, sizeof(src->music_url), "%s", g_music_url_info);
	}
	else if (get_infos_ret == ESP_ERR_NVS_NOT_FOUND)
	{
		player_mdware_play_tone(FLASH_MUSIC_B_04_THE_COLLECTION_LIST_IS_EMPTY);
//		EspAudioPlay(tone_uri[TONE_TYPE_04_THE_COLLECTION_LIST_IS_EMPTY]);
	}
	else
	{
		get_infos_ret = get_collection_infos();
		if(get_infos_ret == ESP_OK)
		{
			snprintf(src->music_url, sizeof(src->music_url), "%s", g_music_url_info);
		}
		else if (get_infos_ret == ESP_ERR_NVS_NOT_FOUND)
		{
			vTaskDelay(3000);
			player_mdware_play_tone(FLASH_MUSIC_B_04_THE_COLLECTION_LIST_IS_EMPTY);
//			EspAudioPlay(tone_uri[TONE_TYPE_04_THE_COLLECTION_LIST_IS_EMPTY]);
			vTaskDelay(3000);
		}
		else
		{
			printf("in get_current_music error play!\n");
			vTaskDelay(3000);
			player_mdware_play_tone(FLASH_MUSIC_B_21_ERROR_PLEASE_TRY_AGAIN_LATER);
//			EspAudioPlay(tone_uri[TONE_TYPE_21_ERROR_PLEASE_TRY_AGAIN_LATER]);
			vTaskDelay(3000);
		}
	}
}

void play_function_start()
{
	int get_infos_ret = ESP_FAIL;
	get_infos_ret = get_collection_infos();
	if(get_infos_ret == ESP_OK)
	{
		AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_music, get_next_music, get_current_music};
		set_auto_play_cb(&call_backs);
	}
	else if (get_infos_ret == ESP_ERR_NVS_NOT_FOUND)
	{
		player_mdware_play_tone(FLASH_MUSIC_B_04_THE_COLLECTION_LIST_IS_EMPTY);
//		EspAudioPlay(tone_uri[TONE_TYPE_04_THE_COLLECTION_LIST_IS_EMPTY]);
	}
	else
	{
		player_mdware_play_tone(FLASH_MUSIC_B_21_ERROR_PLEASE_TRY_AGAIN_LATER);
//		EspAudioPlay(tone_uri[TONE_TYPE_21_ERROR_PLEASE_TRY_AGAIN_LATER]);
	}
}

/*
void trigger_key_prev()
{
	if(get_get_infos_ret() == ESP_OK)
	{
		printf("in trigger_key_prev 1! 000latest_play_index_num = %d\n", latest_play_index_num);
		latest_play_index_num--;
		printf("000latest_play_index_num = %d\n", latest_play_index_num);
		if (latest_play_index_num < PLAY_INDEX_NUM_ONE)
		{
			printf("in trigger_key_prev 2\n");
			latest_play_index_num = PLAY_INDEX_NUM_TWENTY;
			printf("000latest_play_index_num not change! latest_play_index_num = %d\n", latest_play_index_num);
		}
//		get_url();
//		if (strcmp (g_music_url_info, "") != 0)
//		if (g_music_url_info != NULL && strlen(g_music_url_info) >= 10)
		if (get_url() == ESP_OK)
		{
			printf("in trigger_key_prev 3\n");
			if (EspAudioPlay(g_music_url_info) == ESP_OK)
			{
				save_play_url_index();
			}
			else
			{
				EspAudioPlay(tone_uri[TONE_TYPE_21_ERROR_PLEASE_TRY_AGAIN_LATER]);
			}
		}
		else
		{
			printf("in trigger_key_prev 4\n");
			PLAY_INDEX_NUM _latest_play_index_num = latest_play_index_num;
			while (1)
			{
				latest_play_index_num--;
				printf("latest_play_index_num = %d\n", latest_play_index_num);
				if (latest_play_index_num < PLAY_INDEX_NUM_ONE)
				{
					latest_play_index_num = PLAY_INDEX_NUM_TWENTY;
					printf("latest_play_index_num not change! latest_play_index_num = %d\n", latest_play_index_num);
				}
//				get_url();
//				if (strcmp (g_music_url_info, "") != 0)
//				if (g_music_url_info != NULL && strlen(g_music_url_info) != 0)
				if (get_url() == ESP_OK)
				{
					printf("in trigger_key_prev 5\n");
					if (EspAudioPlay(g_music_url_info) == ESP_OK)
					{
						save_play_url_index();
						break;
					}
					else
					{	
						if (EspAudioPlay(g_music_url_info) == ESP_OK)
						{
							save_play_url_index();
							break;
						}
						else
						{
							EspAudioPlay(tone_uri[TONE_TYPE_21_ERROR_PLEASE_TRY_AGAIN_LATER]);
						}
					}
				}
				if (latest_play_index_num == _latest_play_index_num)
				{
					printf("in trigger_key_prev 6\n");
					EspAudioPlay(tone_uri[TONE_TYPE_04_THE_COLLECTION_LIST_IS_EMPTY]);
					break;
				}
			}
		}
	}
	else
	{
		EspAudioPlay(tone_uri[TONE_TYPE_21_ERROR_PLEASE_TRY_AGAIN_LATER]);
	}
}

void trigger_key_next()
{
	if(get_get_infos_ret() == ESP_OK)
	{
		printf("in trigger_key_next 1! 000latest_play_index_num = %d\n", latest_play_index_num);
		latest_play_index_num++;
		printf("000latest_play_index_num = %d\n", latest_play_index_num);
		if (latest_play_index_num > PLAY_INDEX_NUM_TWENTY)
		{
			printf("in trigger_key_next 2\n");
			latest_play_index_num = PLAY_INDEX_NUM_ONE;
			printf("000latest_play_index_num not change! latest_play_index_num = %d\n", latest_play_index_num);
		}
//		get_url();
//		if (strcmp (g_music_url_info, "") != 0)
//		if (g_music_url_info != NULL && strlen(g_music_url_info) >= 10)
		if (get_url() == ESP_OK)
		{
			printf("in trigger_key_next 3\n");
			if (EspAudioPlay(g_music_url_info) == ESP_OK)
			{
				save_play_url_index();
			}
			else
			{
				EspAudioPlay(tone_uri[TONE_TYPE_21_ERROR_PLEASE_TRY_AGAIN_LATER]);
			}
		}
		else
		{
			printf("in trigger_key_next 4\n");
			PLAY_INDEX_NUM _latest_play_index_num = latest_play_index_num;
			while (1)
			{
				latest_play_index_num++;
				printf("latest_play_index_num = %d\n", latest_play_index_num);
				if (latest_play_index_num > PLAY_INDEX_NUM_TWENTY)
				{
					latest_play_index_num = PLAY_INDEX_NUM_ONE;
					printf("latest_play_index_num not change! latest_play_index_num = %d\n", latest_play_index_num);
				}
//				get_url();
//				if (strcmp (g_music_url_info, "") != 0)
//				if (g_music_url_info != NULL && strlen(g_music_url_info) != 0)
				if (get_url() == ESP_OK)
				{
					printf("in trigger_key_next 5\n");
					if (EspAudioPlay(g_music_url_info) == ESP_OK)
					{
						save_play_url_index();
						break;
					}
					else
					{	
						if (EspAudioPlay(g_music_url_info) == ESP_OK)
						{
							save_play_url_index();
							break;
						}
						else
						{
							EspAudioPlay(tone_uri[TONE_TYPE_21_ERROR_PLEASE_TRY_AGAIN_LATER]);
						}
					}
				}
				if (latest_play_index_num == _latest_play_index_num)
				{
					printf("in trigger_key_next 6\n");
					EspAudioPlay(tone_uri[TONE_TYPE_04_THE_COLLECTION_LIST_IS_EMPTY]);
					break;
				}
			}
		}
	}
	else
	{
		EspAudioPlay(tone_uri[TONE_TYPE_21_ERROR_PLEASE_TRY_AGAIN_LATER]);
	}
}
*/
void print_saved_url()
{
	int ret = 0;
	PLAY_INDEX_NUM _latest_play_index_num = latest_play_index_num;
	
	while (1)
	{
		if (latest_play_index_num <= PLAY_INDEX_NUM_TWENTY)
		{
			ret = get_url();
			if (ret == ESP_OK)
			{
				//printf ("\nmusic_url[%d] is :{%s}\n************************************************\n", latest_play_index_num, g_music_url_info);
			}
			else if (ret == ESP_FAIL)
			{
				//printf ("\nin[%d]index,get faild!\n***************************************************<<get faild>>************************\n", latest_play_index_num);
			}
			else
			{
				//printf ("\nin[%d]index,flash is NULL!\n**************************************************<<NULL>>***********************\n", latest_play_index_num);
			}
		}
		else
		{
			latest_play_index_num = _latest_play_index_num;
			get_url();
			break;
		}
		latest_play_index_num++;
	}
	//printf("latest_play_index_num = [%d], save_index_num = [%d], latest_play_index_num g_music_url_info is {%s}\n", latest_play_index_num, save_index_num, g_music_url_info);
}

int init_collection_function()
{
	esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) 
	{
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
	
	g_music_url = esp32_malloc(URL_SIZE);
	if (g_music_url == NULL)
	{
		printf("g_music_url malloc faild!\n");
		return ESP_FAIL;
	}
	g_music_url_info = esp32_malloc(URL_SIZE);
	if (g_music_url_info == NULL)
	{
		printf("g_music_url_info malloc faild!\n");
		return ESP_FAIL;
	}

	err = get_collection_infos(); 
	if (err == ESP_ERR_NVS_NOT_FOUND)
	{
		set_get_infos_ret(ESP_ERR_NVS_NOT_FOUND);
		return ESP_OK;
	}
	else if(err == ESP_FAIL)
	{
		set_get_infos_ret(ESP_FAIL);
		return ESP_FAIL;
	}
	set_get_infos_ret(ESP_OK);
//	print_saved_url();
	return ESP_OK;
}