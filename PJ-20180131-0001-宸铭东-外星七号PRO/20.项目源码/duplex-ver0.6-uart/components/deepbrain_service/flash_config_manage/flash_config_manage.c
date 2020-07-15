#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "flash_config_manage.h"
#include "esp_log.h"
#include "device_api.h"
#include "userconfig.h"
#include "asr_service.h"

static DEVICE_PARAMS_CONFIG_T *g_device_params = NULL;
static PLATFORM_AI_ACOUNTS_T *g_ai_acounts = NULL;
static SemaphoreHandle_t g_lock_flash_config;
static int g_asr_mode = ASR_RECORD_TYPE_AMRWB;

void print_wifi_infos(DEVICE_WIFI_INFOS_T* _wifi_infos)
{
	int i = 0;
	
	if (_wifi_infos == NULL)
	{
		return;
	}

	ESP_LOGE(DEVICE_PARAMS_NAMESPACE, ">>>>>>>>wifi info>>>>>>>>");
	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "connect_index[%d],storage_index[%d]",
		_wifi_infos->wifi_connect_index+1, _wifi_infos->wifi_storage_index+1);
	
	for (i=0; i < MAX_WIFI_NUM; i++)
	{
		ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "[%02d]:ssid[%s]:password[%s]", 
			i+1, _wifi_infos->wifi_info[i].wifi_ssid, _wifi_infos->wifi_info[i].wifi_passwd);
	}
	ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "<<<<<<<<<<<<<<<<<<<<<<<<<");
}

void print_basic_info(DEVICE_BASIC_INFO_T *_basic_info)
{
	if (_basic_info == NULL)
	{
		return;
	}

	ESP_LOGE(DEVICE_PARAMS_NAMESPACE, ">>>>>>>>basic info>>>>>>>");
	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "bind user id:%s", _basic_info->bind_user_id);
	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "device sn:%s", _basic_info->device_sn);
	ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "<<<<<<<<<<<<<<<<<<<<<<<<<");
}

void print_other_params(DEVICE_PARAMS_CONFIG_T *_other_info)
{
	if (_other_info == NULL)
	{
		return;
	}
	
	ESP_LOGE(DEVICE_PARAMS_NAMESPACE, ">>>>>>>>other info>>>>>>>");
	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "flash music index:%d", _other_info->flash_music_index);
	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "sd music index:%d", _other_info->sd_music_folder_index);
	if (_other_info->ota_mode == OTA_UPDATE_MODE_FORMAL)
	{
		ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "ota mode:正式地址[0x%x]", _other_info->ota_mode);
	}
	else if (_other_info->ota_mode == OTA_UPDATE_MODE_TEST)
	{
		ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "ota mode:测试地址[0x%x]", _other_info->ota_mode);
	}
	else
	{
		ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "ota mode:正式地址[0x%x]", _other_info->ota_mode);
	}
	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, ">>>>>>>>other info>>>>>>>");
	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "asr mode:%d", g_asr_mode);
	ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "<<<<<<<<<<<<<<<<<<<<<<<<<");
}

void print_device_params(void)
{
	xSemaphoreTake(g_lock_flash_config, portMAX_DELAY);
	ESP_LOGE(DEVICE_PARAMS_NAMESPACE, ">>>>>>params version>>>>>>");	
	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "param size:%d", sizeof(DEVICE_PARAMS_CONFIG_T));
	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "param version:%d", g_device_params->params_version);
	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "sdk version:%s", ESP32_FW_VERSION);
	ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "<<<<<<<<<<<<<<<<<<<<<<<<<");

	print_wifi_infos(&g_device_params->wifi_infos);
	print_basic_info(&g_device_params->basic_info);
	print_other_params(g_device_params);
	
	xSemaphoreGive(g_lock_flash_config);
}

esp_err_t save_device_params(void)
{
	nvs_handle my_handle;
	esp_err_t err;
	
	err = nvs_open(DEVICE_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) 
	{
		ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "save_device_params nvs_open fail");
		nvs_close(my_handle);
		return err;
	}
	
	err = nvs_set_blob(my_handle, DEVICE_PARAMS_NAME, g_device_params, sizeof(DEVICE_PARAMS_CONFIG_T));
	if (err != ESP_OK) 
	{
		ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "save_device_params nvs_set_blob 1 %s[%d] fail", 
				DEVICE_PARAMS_NAMESPACE, sizeof(DEVICE_PARAMS_CONFIG_T));
		err = nvs_set_blob(my_handle, DEVICE_PARAMS_NAME, g_device_params, sizeof(DEVICE_PARAMS_CONFIG_T));
		if (err != ESP_OK) 
		{
			ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "save_device_params nvs_set_blob 2 %s[%d] fail", 
				DEVICE_PARAMS_NAMESPACE, sizeof(DEVICE_PARAMS_CONFIG_T));
			nvs_close(my_handle);
			return err;
		}
	}
	
	err = nvs_commit(my_handle);
	if (err != ESP_OK) 
	{
		ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "save_device_params nvs_commit fail");
		nvs_close(my_handle);
		return err;
	}
	
	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t get_flash_cfg(FLASH_CONFIG_PARAMS_T _params, void *_value)
{
	esp_err_t ret = ESP_OK;

	xSemaphoreTake(g_lock_flash_config, portMAX_DELAY);
	switch (_params)
	{
		case FLASH_CFG_START:
		{
			break;
		}
		case FLASH_CFG_FLASH_MUSIC_INDEX:
		{
			*(int*)_value = g_device_params->flash_music_index;
			break;
		}
		case FLASH_CFG_SDCARD_FOLDER_INDEX:
		{
			*(int*)_value = g_device_params->sd_music_folder_index;
			break;
		}
		case FLASH_CFG_WIFI_INFO:
		{
			memcpy((char*)_value, (char*)&g_device_params->wifi_infos, sizeof(g_device_params->wifi_infos));
			break;
		}
		case FLASH_CFG_USER_ID:
		{
			snprintf((char*)_value, sizeof(g_device_params->basic_info.bind_user_id), 
				"%s", g_device_params->basic_info.bind_user_id);
			break;
		}
		case FLASH_CFG_DEVICE_ID:
		{
			snprintf((char*)_value, sizeof(g_device_params->basic_info.device_sn), 
				"%s", g_device_params->basic_info.device_sn);
			break;
		}
		case FLASH_CFG_MEMO_PARAMS:
		{
			memcpy((MEMO_ARRAY_T *)_value, (MEMO_ARRAY_T *)&g_device_params->memo_array, sizeof(MEMO_ARRAY_T));
			break;
		}
		case FLASH_CFG_OTA_MODE:
		{
			if (g_device_params->ota_mode == OTA_UPDATE_MODE_FORMAL)
			{
				*(uint32_t*)_value = OTA_UPDATE_MODE_FORMAL;
			}
			else if (g_device_params->ota_mode == OTA_UPDATE_MODE_TEST)
			{
				*(uint32_t*)_value = OTA_UPDATE_MODE_TEST;
			}
			else
			{
				*(uint32_t*)_value = OTA_UPDATE_MODE_FORMAL;
			}
			break;
		}
		case FLASH_CFG_ASR_MODE:
		{
			*(uint32_t*)_value = g_asr_mode;
			break;
		}
		case FLASH_CFG_END:
		{
			break;
		}
		default:
			break;
	}
	xSemaphoreGive(g_lock_flash_config);

	return ret;
}


esp_err_t set_flash_cfg(FLASH_CONFIG_PARAMS_T _params, void *_value)
{
	esp_err_t ret = ESP_OK;

	xSemaphoreTake(g_lock_flash_config, portMAX_DELAY);
	switch (_params)
	{
		case FLASH_CFG_START:
		{
			break;
		}
		case FLASH_CFG_FLASH_MUSIC_INDEX:
		{
			if (g_device_params->flash_music_index != *(int*)_value)
			{
				g_device_params->flash_music_index = *(int*)_value;
				ret = save_device_params();
				if (ret != ESP_OK)
				{
					ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "save_device_params FLASH_CFG_FLASH_MUSIC_INDEX fail");
				}
			}
			break;
		}
		case FLASH_CFG_SDCARD_FOLDER_INDEX:
		{
			if (g_device_params->sd_music_folder_index != *(int*)_value)
			{
				g_device_params->sd_music_folder_index = *(int*)_value;
				ret = save_device_params();
				if (ret != ESP_OK)
				{
					ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "save_device_params FLASH_CFG_SDCARD_FOLDER_INDEX fail");
				}
			}
			break;
		}
		case FLASH_CFG_WIFI_INFO:
		{
			if (memcmp((char*)&g_device_params->wifi_infos, (char*)_value, sizeof(g_device_params->wifi_infos)) != 0)
			{
				memcpy((char*)&g_device_params->wifi_infos, (char*)_value, sizeof(g_device_params->wifi_infos));
				ret = save_device_params();
				if (ret != ESP_OK)
				{
					ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "save_device_params FLASH_CFG_WIFI_INFO fail");
				}
			}
			break;
		}
		case FLASH_CFG_USER_ID:
		{
			if (strcmp(g_device_params->basic_info.bind_user_id, (char*)_value) != 0)
			{
				snprintf((char*)&g_device_params->basic_info.bind_user_id, sizeof(g_device_params->basic_info.bind_user_id),
					"%s", (char*)_value);
				ret = save_device_params();
				if (ret != ESP_OK)
				{
					ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "save_device_params FLASH_CFG_USER_ID fail[0x%x]", ret);
				}
			}
			break;
		}
		case FLASH_CFG_DEVICE_ID:
		{
			if (strcmp(g_device_params->basic_info.device_sn, (char*)_value) != 0)
			{
				snprintf((char*)&g_device_params->basic_info.device_sn, sizeof(g_device_params->basic_info.device_sn),
					"%s", (char*)_value);
				ret = save_device_params();
				if (ret != ESP_OK)
				{
					ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "save_device_params FLASH_CFG_DEVICE_ID fail[0x%x]", ret);
				}
			}
			break;
		}
		case FLASH_CFG_MEMO_PARAMS:
		{
			memcpy((MEMO_ARRAY_T *)&g_device_params->memo_array, (MEMO_ARRAY_T *)_value, sizeof(MEMO_ARRAY_T));
			ret = save_device_params();
			if (ret != ESP_OK)
			{
				ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "save_device_params FLASH_CFG_MEMO_PARAMS fail");
			}
		}
		case FLASH_CFG_OTA_MODE:
		{
			if (g_device_params->ota_mode != *(uint32_t*)_value)
			{
				if (*(uint32_t*)_value == OTA_UPDATE_MODE_FORMAL)
				{
					g_device_params->ota_mode = OTA_UPDATE_MODE_FORMAL;
				}
				else if (*(uint32_t*)_value == OTA_UPDATE_MODE_TEST)
				{
					g_device_params->ota_mode = OTA_UPDATE_MODE_TEST;
				}
				else
				{
					g_device_params->ota_mode = OTA_UPDATE_MODE_FORMAL;
				}
				
				ret = save_device_params();
				if (ret != ESP_OK)
				{
					ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "save_device_params FLASH_CFG_OTA_MODE fail");
				}
			}
			break;
		}
		case FLASH_CFG_ASR_MODE:
		{
			int mode = *(uint32_t*)_value;
			if (mode >= ASR_RECORD_TYPE_AMRNB && mode <= ASR_RECORD_TYPE_PCM_SINOVOICE_16K)
			{
				g_asr_mode = mode;
			}
			break;
		}
		case FLASH_CFG_END:
		{
			break;
		}
		default:
			break;
	}
	xSemaphoreGive(g_lock_flash_config);
	
	return ret;
}

int init_device_params_v1(nvs_handle _handle, DEVICE_PARAMS_CONFIG_T *_config)
{
	esp_err_t err;
	
	if (_config == NULL)
	{
		ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "_config is NULL");
		return ESP_FAIL;
	}

	memset(_config, 0, sizeof(DEVICE_PARAMS_CONFIG_T));
	_config->params_version = DEVICE_PARAMS_VERSION_1;
	snprintf(_config->wifi_infos.wifi_info[0].wifi_ssid, sizeof(_config->wifi_infos.wifi_info[0].wifi_ssid),
		DEFAULT_WIFI_SSID);
	snprintf(_config->wifi_infos.wifi_info[0].wifi_passwd, sizeof(_config->wifi_infos.wifi_info[0].wifi_passwd),
		DEFAULT_WIFI_PASSWD);

	ESP_ERROR_CHECK(nvs_erase_all(_handle));
	err = nvs_set_blob(_handle, DEVICE_PARAMS_NAME, _config, sizeof(DEVICE_PARAMS_CONFIG_T));
	if (err != ESP_OK) 
	{
		ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "nvs_set_blob %s first fail(%x)", DEVICE_PARAMS_NAMESPACE, err);
		return err;
	}
	
	err = nvs_commit(_handle);
	if (err != ESP_OK) 
	{
		ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "nvs_commit %s first fail(%x)", DEVICE_PARAMS_NAMESPACE, err);
		return err;
	}

	return ESP_OK;
}

void init_default_ai_sinovoice_acount(SINOVOICE_ACOUNT_T *_sinovice_acount)
{
	if (_sinovice_acount == NULL)
	{
		return;
	}

	snprintf(_sinovice_acount->asr_url, sizeof(_sinovice_acount->asr_url), "%s", SINOVOICE_DEFAULT_ASR_URL);
	snprintf(_sinovice_acount->tts_url, sizeof(_sinovice_acount->tts_url), "%s", SINOVOICE_DEFAULT_TTS_URL);
	snprintf(_sinovice_acount->app_key, sizeof(_sinovice_acount->app_key), "%s", SINOVOICE_DEFAULT_APP_KEY);
	snprintf(_sinovice_acount->dev_key, sizeof(_sinovice_acount->dev_key), "%s", SINOVOICE_DEFAULT_DEV_KEY);
}

void init_default_ai_baidu_acount(BAIDU_ACOUNT_T *_baidu_acount)
{
	if (_baidu_acount == NULL)
	{
		return;
	}

	snprintf(_baidu_acount->asr_url, sizeof(_baidu_acount->asr_url), "%s", BAIDU_DEFAULT_ASR_URL);
	snprintf(_baidu_acount->tts_url, sizeof(_baidu_acount->tts_url), "%s", BAIDU_DEFAULT_TTS_URL);
	snprintf(_baidu_acount->app_id, sizeof(_baidu_acount->app_id), "%s", BAIDU_DEFAULT_APP_ID);
	snprintf(_baidu_acount->app_key, sizeof(_baidu_acount->app_key), "%s", BAIDU_DEFAULT_APP_KEY);
	snprintf(_baidu_acount->secret_key, sizeof(_baidu_acount->secret_key), "%s", BAIDU_DEFAULT_SECRET_KEY);
}

void init_default_ai_acounts(PLATFORM_AI_ACOUNTS_T *_accounts)
{
	if (_accounts == NULL)
	{
		return;
	}

	memset(_accounts, 0, sizeof(PLATFORM_AI_ACOUNTS_T));
	
	init_default_ai_sinovoice_acount(&_accounts->st_sinovoice_acount);
	init_default_ai_baidu_acount(&_accounts->st_baidu_acount);
}

int init_device_params(void)
{
	nvs_handle my_handle;
	esp_err_t err;

	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "init_device_params start");
	g_lock_flash_config = xSemaphoreCreateMutex();

	g_ai_acounts = esp32_malloc(sizeof(PLATFORM_AI_ACOUNTS_T));
	if (g_ai_acounts == NULL)
	{
		ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "esp32_malloc g_ai_acounts fail");
		return ESP_FAIL;
	}
	init_default_ai_acounts(g_ai_acounts);
	
	g_device_params = esp32_malloc(sizeof(DEVICE_PARAMS_CONFIG_T));
	if (g_device_params == NULL)
	{
		ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "esp32_malloc g_device_params fail");
		return ESP_FAIL;
	}
	memset(g_device_params, 0, sizeof(DEVICE_PARAMS_CONFIG_T));
	
	err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) 
	{
        ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "nvs_flash_init frist init fail");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "nvs_flash_init first success");

	err = nvs_open(DEVICE_PARAMS_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) 
	{
		ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "nvs_open %s first fail(%x)", DEVICE_PARAMS_NAMESPACE, err);
		return err;
	}
	ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "nvs_open %s success", DEVICE_PARAMS_NAMESPACE);

	size_t len = sizeof(DEVICE_PARAMS_CONFIG_T);
	err = nvs_get_blob(my_handle, DEVICE_PARAMS_NAME, g_device_params, &len);
	switch (err) 
	{
        case ESP_OK:
        {
            ESP_LOGI(DEVICE_PARAMS_NAMESPACE, "nvs_get_blob %s success", DEVICE_PARAMS_NAMESPACE);
			if (g_device_params->params_version != DEVICE_PARAMS_VERSION_1
				&& len != sizeof(DEVICE_PARAMS_CONFIG_T))
			{
				err = init_device_params_v1(my_handle, g_device_params);
				if (err != ESP_OK)
				{
					nvs_close(my_handle);
					ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "init_device_params_v1 first fail");
					return ESP_FAIL;
				}
			}
			break;
        }
        case ESP_ERR_NVS_NOT_FOUND:
        {
            ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "nvs_get_blob %s ESP_ERR_NVS_NOT_FOUND", DEVICE_PARAMS_NAMESPACE);
			err = init_device_params_v1(my_handle, g_device_params);
			if (err != ESP_OK)
			{
				nvs_close(my_handle);
				ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "init_device_params_v1 second fail");
				return ESP_FAIL;
			}
            break;
        }
        default :
            ESP_LOGE(DEVICE_PARAMS_NAMESPACE, "Error (%x) reading!\n", err);
    }

	nvs_close(my_handle);

	print_device_params();
	
	return err;
}

void get_ai_acount(AI_ACOUNT_T _type, void *_out)
{
	if (g_ai_acounts == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_lock_flash_config, portMAX_DELAY);
	switch (_type)
	{
		case AI_ACOUNT_BAIDU:
		{
			if (g_ai_acounts == NULL)
			{
				init_default_ai_baidu_acount((BAIDU_ACOUNT_T *)_out);
			}
			else
			{
				memcpy(_out, &g_ai_acounts->st_baidu_acount, sizeof(g_ai_acounts->st_baidu_acount));
			}
			break;
		}
		case AI_ACOUNT_SINOVOICE:
		{
			if (g_ai_acounts == NULL)
			{
				init_default_ai_sinovoice_acount((SINOVOICE_ACOUNT_T *)_out);
			}
			else
			{
				memcpy(_out, &g_ai_acounts->st_sinovoice_acount, sizeof(g_ai_acounts->st_sinovoice_acount));
			}
			break;
		}
		default:
			break;
	}
	xSemaphoreGive(g_lock_flash_config);
}

void set_ai_acount(AI_ACOUNT_T _type, void *_out)
{
	if (g_ai_acounts == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_lock_flash_config, portMAX_DELAY);
	switch (_type)
	{
		case AI_ACOUNT_BAIDU:
		{
			memcpy(&g_ai_acounts->st_baidu_acount, _out, sizeof(g_ai_acounts->st_baidu_acount));
			break;
		}
		case AI_ACOUNT_SINOVOICE:
		{
			memcpy(&g_ai_acounts->st_sinovoice_acount, _out, sizeof(g_ai_acounts->st_sinovoice_acount));
			break;
		}
		case AI_ACOUNT_ALL:
		{
			memcpy(g_ai_acounts, _out, sizeof(PLATFORM_AI_ACOUNTS_T));
			break;
		}
		default:
			break;
	}
	xSemaphoreGive(g_lock_flash_config);
}


