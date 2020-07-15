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
#include "led.h"
#include "player_middleware.h"
#include "interf_enc.h"
#include "mpush_service.h"
#include "bind_device.h"
#include "ReSampling.h"
#include "amrwb_encode.h"

#define LOG_TAG "wechat service"

static WECHAT_SERVICE_HANDLE_T *g_wechat_handle = NULL;

static void wechat_push_process(WECHAT_MSG_T *p_msg)
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
			player_mdware_play_tone(FLASH_MUSIC_WECHAT_SHORT_AUDIO);
			break;
		}
		case ASR_RESULT_TYPE_SUCCESS:
		case ASR_RESULT_TYPE_FAIL:
		case ASR_RESULT_TYPE_NO_RESULT:
		{
			uint64_t start_time = get_time_of_day();
			//微聊推送
			ret = mpush_service_send_file(p_msg->asr_result.record_data, p_msg->asr_result.record_len, p_msg->asr_result.asr_text, "amr", "");
			if (ret == MPUSH_ERROR_SEND_MSG_OK)
			{
				player_mdware_play_tone(FLASH_MUSIC_SEND_SUCCESS);
				ESP_LOGI(LOG_TAG, "mpush_service_send_file success"); 
			}
			else
			{
				player_mdware_play_tone(FLASH_MUSIC_SEND_FAIL);	
				ESP_LOGE(LOG_TAG, "mpush_service_send_file failed"); 
			}
			ESP_LOGI(LOG_TAG, "[wechat push time]:cost[%lldms]", get_time_of_day() - start_time);
			break;
		}
		default:
			break;
	}
}

static void task_wechat(void *pv)
{
	int ret = 0;
	int i = 0;
	WECHAT_MSG_T *p_msg = NULL;
    WECHAT_SERVICE_HANDLE_T *wechat_handle = (WECHAT_SERVICE_HANDLE_T *)pv;
	
    while (1) 
	{
		if (xQueueReceive(wechat_handle->msg_queue, &p_msg, portMAX_DELAY) == pdPASS) 
		{			
			switch (p_msg->msg_type)
			{
				case WECHAT_MSG_TYPE_ASR_RET:
				{
					wechat_push_process(p_msg);
					break;
				}
				case WECHAT_MSG_TYPE_QUIT:
				{
					break;
				}
				default:
					break;
			}

			if (p_msg->msg_type == WECHAT_MSG_TYPE_QUIT)
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

static void wechat_asr_result_cb(ASR_RESULT_T *result)
{
	WECHAT_MSG_T *p_msg = NULL;
	
	if (g_wechat_handle == NULL || g_wechat_handle->msg_queue == NULL)
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
	if (xQueueSend(g_wechat_handle->msg_queue, &p_msg, 0) != pdPASS)
	{
		esp32_free(p_msg);
		p_msg = NULL;
	}
	
	return;
}

static void set_wechat_record_data(
	int record_sn, int id, char *data, size_t data_len, int record_ms, int is_max)
{
	ASR_RECORD_MSG_T *p_msg = NULL;

	if (g_wechat_handle == NULL || g_wechat_handle->msg_queue == NULL)
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
	p_msg->record_data.asr_engine = ASR_ENGINE_TYPE_AMRNB;
	p_msg->asr_call_back = wechat_asr_result_cb;
	
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


void wechat_service_create(void)
{
	g_wechat_handle = (WECHAT_SERVICE_HANDLE_T *)esp32_malloc(sizeof(WECHAT_SERVICE_HANDLE_T));
	if (g_wechat_handle == NULL)
	{
		return;
	}
	
	g_wechat_handle->msg_queue = xQueueCreate(2, sizeof(char *));
	
    if (xTaskCreate(task_wechat,
                    "task_wechat",
                    1024*4,
                    g_wechat_handle,
                    4,
                    NULL) != pdPASS) {

        ESP_LOGE(LOG_TAG, "ERROR creating task_wechat task! Out of memory?");
    }
}

void wechat_service_delete(void)
{
	
}

void wechat_record_start(void)
{
	player_mdware_start_record(1, ASR_AMR_MAX_TIME_MS, set_wechat_record_data);
}

void wechat_record_stop(void)
{
	player_mdware_stop_record(set_wechat_record_data);
}



