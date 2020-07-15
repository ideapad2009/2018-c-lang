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

#include "cJSON.h"
#include "MediaHal.h"
#include "volume_manage.h"
#include "player_middleware.h"

#define VOLUME_TAG			"volume_manage"

static void get_vol_ctrl_cmd(cJSON *pJson_cmd_attrs_array_item, VOLUME_CONTROL_T *volume_control_state)
{
	cJSON *pJson_attr_value_type = cJSON_GetObjectItem(pJson_cmd_attrs_array_item, "attrValue");
	if (pJson_attr_value_type != NULL && pJson_attr_value_type->valuestring != NULL)
	{		
		if (strcmp(pJson_attr_value_type->valuestring, "调高") == 0)
		{
			*volume_control_state = VOLUME_CONTROL_HIGHER;
		}
		if (strcmp(pJson_attr_value_type->valuestring, "调低") == 0)
		{
			*volume_control_state = VOLUME_CONTROL_LOWER;
		}
		if (strcmp(pJson_attr_value_type->valuestring, "适中") == 0)
		{
			*volume_control_state = VOLUME_CONTROL_MODERATE;
		}
		if (strcmp(pJson_attr_value_type->valuestring, "最大") == 0)
		{
			*volume_control_state = VOLUME_CONTROL_MAX;
		}
		if (strcmp(pJson_attr_value_type->valuestring, "最小") == 0)
		{
			*volume_control_state = VOLUME_CONTROL_MIN;
		}			
	}
	else
	{
		*volume_control_state = VOLUME_CONTROL_INITIAL;
		DEBUG_LOGE(VOLUME_TAG, "pJson_attr_value_type wrong\n");	
	}			
}

static void get_vol_scale_need_to_change(cJSON *pJson_cmd_attrs_array_item, VOLUME_TYPE_T *volume_type, int *scale)
{
	cJSON *pJson_attr_value_scale = cJSON_GetObjectItem(pJson_cmd_attrs_array_item, "attrValue");
	if (pJson_attr_value_scale != NULL && pJson_attr_value_scale->valuestring != NULL)
	{
		if (strcmp(pJson_attr_value_scale->valuestring, "") != 0)
		{
			*scale = atoi(pJson_attr_value_scale->valuestring);
			*volume_type = VOLUME_TYPE_SCALE_MODE;
		}
		else
		{
			*volume_type = VOLUME_TYPE_NOT_SCALE_MODE;
		}
	}
}

static int choose_vol_value(VOLUME_TYPE_T volume_type, int constant_vol_value, int scale_mode_vol_value)
{
	if (volume_type == VOLUME_TYPE_SCALE_MODE)
	{
		return scale_mode_vol_value;
	}
	else
	{
		return constant_vol_value;
	}
}

static void volume_control_process(VOLUME_CONTROL_T volume_control_state, int volume_change_value)
{
	int current_vol = 0;
	
	if (MediaHalGetVolume(&current_vol) != 0)
	{
		if (MediaHalGetVolume(&current_vol) != 0)
		{
			ESP_LOGE(VOLUME_TAG, "GetVolume failed! (line %d)", __LINE__);
			player_mdware_play_tone(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER);
			return;
		}
	}
	ESP_LOGI(VOLUME_TAG, "now_vol = %d", current_vol);
	
	switch (volume_control_state)
	{
		case VOLUME_CONTROL_HIGHER:	//增大音量
		{
			ESP_LOGI(VOLUME_TAG, "vol up");
        	int need_to_set_vol = current_vol + volume_change_value;
			if (need_to_set_vol > MAX_VOLUME)
			{
				if (MediaHalSetVolume(MAX_VOLUME) != 0)
				{
	                if (MediaHalSetVolume(MAX_VOLUME) != 0)
					{
						ESP_LOGE(VOLUME_TAG, "SetVolume failed! (line %d)", __LINE__);
						player_mdware_play_tone(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER);
						break;
		            }
	            }
				ESP_LOGI(VOLUME_TAG, "after set vol = %d (line %d)", MAX_VOLUME, __LINE__);
				player_mdware_play_tone(FLASH_MUSIC_DYY_VOL_MAX);//音频："已经最大了"
			}
			else
			{
				if (MediaHalSetVolume(need_to_set_vol) != 0)
				{
	                if (MediaHalSetVolume(need_to_set_vol) != 0)
					{
						ESP_LOGE(VOLUME_TAG, "SetVolume failed! (line %d)", __LINE__);
						player_mdware_play_tone(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER);
						break;
		            }
	            }
				ESP_LOGI(VOLUME_TAG, "after vol = %d (line %d)", need_to_set_vol, __LINE__);
				player_mdware_play_tone(FLASH_MUSIC_DYY_VOLUME_UP);//音频："已经调大了"
			}
			break;
		}
		case VOLUME_CONTROL_LOWER:	//减小音量
		{
			ESP_LOGI(VOLUME_TAG, "vol down");
        	int need_to_set_vol = current_vol - volume_change_value;
			if (need_to_set_vol < MIN_VOLUME)
			{
				if (MediaHalSetVolume(MIN_VOLUME) != 0)
				{
	                if (MediaHalSetVolume(MIN_VOLUME) != 0)
					{
						ESP_LOGE(VOLUME_TAG, "SetVolume failed! (line %d)", __LINE__);
						player_mdware_play_tone(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER);
						break;
		            }
	            }
				ESP_LOGI(VOLUME_TAG, "after set vol = %d (line %d)", MIN_VOLUME, __LINE__);
				player_mdware_play_tone(FLASH_MUSIC_DYY_VOL_MIN);//音频："已经最小了"
			}
			else
			{
				if (MediaHalSetVolume(need_to_set_vol) != 0)
				{
	                if (MediaHalSetVolume(need_to_set_vol) != 0)
					{
						ESP_LOGE(VOLUME_TAG, "SetVolume failed! (line %d)", __LINE__);
						player_mdware_play_tone(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER);
						break;
		            }
	            }
				ESP_LOGI(VOLUME_TAG, "after set vol = %d (line %d)", need_to_set_vol, __LINE__);
				player_mdware_play_tone(FLASH_MUSIC_DYY_VOLUME_DOWN);//音频："已经调小了"
			}
			break;
		}
		case VOLUME_CONTROL_MODERATE:	//音量适中
		{
			ESP_LOGI(VOLUME_TAG, "vol moderate");
			if (MediaHalSetVolume(MODERATE_VOLUME) != 0)
			{
                if (MediaHalSetVolume(MODERATE_VOLUME) != 0)
				{
					ESP_LOGE(VOLUME_TAG, "SetVolume failed! (line %d)", __LINE__);
					player_mdware_play_tone(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER);
					break;
	            }
            }
			ESP_LOGI(VOLUME_TAG, "after set vol = %d (line %d)", MODERATE_VOLUME, __LINE__);
			player_mdware_play_tone(FLASH_MUSIC_DYY_VOLUME_CHANGE_OK);//音频："已经调好了"
			break;
		}
		case VOLUME_CONTROL_MAX:	//音量最大
		{
			ESP_LOGI(VOLUME_TAG, "vol max");
			if (MediaHalSetVolume(MAX_VOLUME) != 0)
			{
                if (MediaHalSetVolume(MAX_VOLUME) != 0)
				{
					ESP_LOGE(VOLUME_TAG, "SetVolume failed! (line %d)", __LINE__);
					player_mdware_play_tone(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER);
					break;
	            }
            }
			ESP_LOGI(VOLUME_TAG, "after set vol = %d (line %d)", MAX_VOLUME, __LINE__);
			player_mdware_play_tone(FLASH_MUSIC_DYY_VOLUME_ALREADY_HIGHEST);//音频："音量已最大"
			break;
		}
		case VOLUME_CONTROL_MIN:	//音量最小
		{
			ESP_LOGI(VOLUME_TAG, "vol min");
			if (MediaHalSetVolume(MIN_VOLUME) != 0)
			{
                if (MediaHalSetVolume(MIN_VOLUME) != 0)
				{
					ESP_LOGE(VOLUME_TAG, "SetVolume failed! (line %d)", __LINE__);
					player_mdware_play_tone(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER);
					break;
	            }
            }
			ESP_LOGI(VOLUME_TAG, "after set vol = %d (line %d)", MIN_VOLUME, __LINE__);
			player_mdware_play_tone(FLASH_MUSIC_DYY_VOLUME_ALREADY_LOWEST);//音频："音量已最小"
			break;
		}
		default:
		{
			player_mdware_play_tone(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER);
			break;
		}
	}
}

void volume_control(cJSON *json_data)
{
	int array_size = 0;
	int volume_change_scale = 0;
	int constant_vol_value = VOL_STEP_VALUE;
	VOLUME_TYPE_T volume_type = VOLUME_TYPE_INITIAL;
	VOLUME_CONTROL_T volume_control_state = VOLUME_CONTROL_INITIAL;
	
	cJSON *pJson_cmd_attrs = cJSON_GetObjectItem(json_data, "commandAttrs");
	if (pJson_cmd_attrs != NULL)
	{
		array_size = cJSON_GetArraySize(pJson_cmd_attrs);
		if (array_size > 0)
		{
			for (int i = 0; i < array_size; i++)
			{
				cJSON *pJson_cmd_attrs_array_item = cJSON_GetArrayItem(pJson_cmd_attrs, i);
				if (pJson_cmd_attrs_array_item != NULL)
				{
					cJSON *pJson_attr_name = cJSON_GetObjectItem(pJson_cmd_attrs_array_item, "attrName");
					if (strcmp(pJson_attr_name->valuestring, ATTR_NAME_VOL_CTRL) == 0)
					{
						get_vol_ctrl_cmd(pJson_cmd_attrs_array_item, &volume_control_state);
					}
					else if (strcmp(pJson_attr_name->valuestring, ATTR_NAME_VOL_SCALE) == 0)
					{
						get_vol_scale_need_to_change(pJson_cmd_attrs_array_item, &volume_type, &volume_change_scale);
					}
				}
				else
				{
					ESP_LOGE(VOLUME_TAG, "pJson failed! (line %d)", __LINE__);
					player_mdware_play_tone(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER);
					return;
				}
			}
		}
		volume_control_process(volume_control_state, choose_vol_value(volume_type, constant_vol_value, volume_change_scale));
	}
}

