#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "userconfig.h"
#include "ota_update.h"
#include "MediaService.h"
#include "ota_service.h"
#include "DeviceCommon.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "EspAudio.h"
#include "toneuri.h"
#include "MediaHal.h"
#include "flash_config_manage.h"
#include "player_middleware.h"
#include "auto_play_service.h"
#include "semaphore_lock.h"
#include "device_api.h"

#define OTA_SERV_TAG "OTA_SERVICE"

OTA_SERVICE_T *g_ota_service_handle = NULL;
static SemaphoreHandle_t g_lock_ota = NULL;

static void DeviceEvtNotifiedToOTA(DeviceNotification *note)
{
    OTA_SERVICE_T *service = (OTA_SERVICE_T *) note->receiver;
    if (DEVICE_NOTIFY_TYPE_WIFI == note->type) {
        DeviceNotifyMsg msg = *((DeviceNotifyMsg *) note->data);
        switch (msg) {
        case DEVICE_NOTIFY_WIFI_GOT_IP:
        {
            ota_service_send_msg(MSG_OTA_WIFI_UPDATE);
            break;
        }
        default:
            break;
        }
    }
}

static void ota_service_play_tone_start(void)
{
	
	OTA_PLAY_TONE_MSG_T msg = OTA_PLAY_TONE_MSG_START;
	
	if (g_ota_service_handle != NULL && g_ota_service_handle->ota_tone_queue != NULL)
	{
		xQueueSend(g_ota_service_handle->ota_tone_queue, &msg, 0);
	}
}

static void ota_service_play_tone_stop(void)
{
	OTA_PLAY_TONE_MSG_T msg = OTA_PLAY_TONE_MSG_STOP;
	
	if (g_ota_service_handle != NULL && g_ota_service_handle->ota_tone_queue != NULL)
	{
		xQueueSend(g_ota_service_handle->ota_tone_queue, &msg, 0);
	}
}

static void task_ota_play_tone(void *pvParameters)
{
	OTA_PLAY_TONE_MSG_T msg = OTA_PLAY_TONE_MSG_NONE;
	OTA_PLAY_TONE_MSG_T status = OTA_PLAY_TONE_MSG_NONE; 
	OTA_SERVICE_T *service  = (OTA_SERVICE_T *) pvParameters;
	
	ESP_LOGE(OTA_SERV_TAG, "task_ota_play_tone run");
    while (1) 
	{
		msg = OTA_PLAY_TONE_MSG_NONE;
        if (xQueueReceive(service->ota_tone_queue, &msg, (TickType_t)2000) == pdPASS)
        {
			switch (msg)
			{
				case OTA_PLAY_TONE_MSG_NONE:
				{
					break;
				}
				case OTA_PLAY_TONE_MSG_START:
				{
					status = OTA_PLAY_TONE_MSG_START;
					auto_play_stop();
					break;
				}
				case OTA_PLAY_TONE_MSG_STOP:
				{
					status = OTA_PLAY_TONE_MSG_STOP;
					break;
				}
				default:
					break;
			}
		}

		switch (status)
		{
			case OTA_PLAY_TONE_MSG_NONE:
			{
				break;
			}
			case OTA_PLAY_TONE_MSG_START:
			{
				player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);	
				break;
			}
			case OTA_PLAY_TONE_MSG_STOP:
			{
				break;
			}
			default:
				break;
		}

		if (status == OTA_PLAY_TONE_MSG_STOP)
		{
			service->task_status_play_tone = OTA_TASK_STATUS_EXIT;
			break;
		}
    }

	ESP_LOGE(OTA_SERV_TAG, "task_ota_play_tone exit");
    vTaskDelete(NULL);
}

static void task_ota_update(void *pvParameters)
{
    OTA_SERVICE_T *service  = (OTA_SERVICE_T *) pvParameters;    
	OTA_UPDATE_MODE_T ota_mode = OTA_UPDATE_MODE_FORMAL;
	char str_fw_url[256] = {0};
	
	ESP_LOGE(OTA_SERV_TAG, "task_ota_update run");
	while(1)
	{
		get_flash_cfg(FLASH_CFG_OTA_MODE, &ota_mode);
		if (ota_mode == OTA_UPDATE_MODE_FORMAL)
		{
			if (ota_update_check_fw_version(OTA_UPDATE_SERVER_URL, str_fw_url, sizeof(str_fw_url)) == OTA_NONEED_UPDATE_FW)
			{
				break;
			}
		}
		else
		{
			if (ota_update_check_fw_version(OTA_UPDATE_SERVER_URL_TEST, str_fw_url, sizeof(str_fw_url)) == OTA_NONEED_UPDATE_FW)
			{
				break;
			}
		}
		service->ota_state = OTA_UPGRADE_STATE_UPGRADING;
		auto_play_stop();
		vTaskDelay(500);	//连网成功语音缓冲时间
		player_mdware_play_tone(FLASH_MUSIC_EQUIPMENT_UPGRADING);
		vTaskDelay(5000);
		ota_service_play_tone_start();
		if (ota_update_from_wifi(str_fw_url) == OTA_SUCCESS)
		{
			ESP_LOGE(OTA_SERV_TAG, "ota_update_from_wifi success");
			ota_service_play_tone_stop();
			vTaskDelay(1000);	//停止后音效缓冲时间
			player_mdware_play_tone(FLASH_MUSIC_UPGRADE_COMPLETE);
			vTaskDelay(3000);
			MediaHalPaPwr(-1);
			esp_restart();
		}
		else
		{
			ota_service_play_tone_stop();
			vTaskDelay(1000);	//停止后音效缓冲时间
			player_mdware_play_tone(FLASH_MUSIC_UPDATE_FAIL);
			vTaskDelay(3000);
			service->ota_state = OTA_UPGRADE_STATE_NOT_UPGRADING;
			ESP_LOGE(OTA_SERV_TAG, "ota_update_from_wifi failed");
		}
		break;
	}
	
	ota_service_play_tone_stop();
	ESP_LOGE(OTA_SERV_TAG, "task_ota_update exit");
	service->task_status_ota_update = OTA_TASK_STATUS_EXIT;
    vTaskDelete(NULL);
}

/*
	1、当网络连接成功后，才启动ota task，这样可以节约很多片内内存。
*/
static void task_ota_manage(void *pvParameters)
{
	MSG_OTA_SERVICE_T msg = MSG_OTA_IDEL;
	OTA_SERVICE_T *service  = (OTA_SERVICE_T *) pvParameters;

	vTaskDelay(10*1000);
	while (1) 
	{
		if (xQueueReceive(service->ota_queue, &msg, portMAX_DELAY) == pdPASS)
		{
			switch (msg)
			{
				case MSG_OTA_WIFI_UPDATE:
				{
					if (service->task_status_ota_update != OTA_TASK_STATUS_RUNING)
					{
						if (xTaskCreatePinnedToCore(task_ota_update,
                                "task_ota_update",
                                6*1024,
                                service,
                                13,
                                NULL, xPortGetCoreID()) != pdPASS) 
					    {
					        ESP_LOGE(OTA_SERV_TAG, "Error create task_ota_update");
							break;
					    }
						service->task_status_ota_update = OTA_TASK_STATUS_RUNING;
					}

					if (service->task_status_play_tone != OTA_TASK_STATUS_RUNING)
					{
						if (xTaskCreate(task_ota_play_tone,
		                    "task_ota_play_tone",
		                    3*1024,
		                    service,
		                    15,
		                    NULL) != pdPASS) 
					    {
					        ESP_LOGE(OTA_SERV_TAG, "Error create task_ota_play_tone");
							break;
					    }
						service->task_status_play_tone = OTA_TASK_STATUS_RUNING;
					}
					break;
				}
				case MSG_OTA_SD_UPDATE:
				{
					break;
				}
				default:
					break;
			}
		}
	}
}

static void OtaServiceActive(OTA_SERVICE_T *service)
{
    ESP_LOGI(OTA_SERV_TAG, "OtaService active");
    if (xTaskCreate(task_ota_manage,
	    "task_ota_manage",
	    1*1024,
	    service,
	    15,
	    NULL) != pdPASS) 
    {
        ESP_LOGE(OTA_SERV_TAG, "Error create task_ota_manage");
    }
}

static void OtaServiceDeactive(OTA_SERVICE_T *service)
{
	SEMPHR_TRY_LOCK(g_lock_ota);
    ESP_LOGI(OTA_SERV_TAG, "OtaServiceStop");
	if (g_ota_service_handle != NULL && g_ota_service_handle->ota_queue != NULL)
	{
		 vQueueDelete(g_ota_service_handle->ota_queue);
		 g_ota_service_handle->ota_queue = NULL;
	}

	if (g_ota_service_handle != NULL && g_ota_service_handle->ota_tone_queue != NULL)
	{
		 vQueueDelete(g_ota_service_handle->ota_tone_queue);
		 g_ota_service_handle->ota_tone_queue = NULL;
	}

	if (g_ota_service_handle != NULL)
	{
		 esp32_free(g_ota_service_handle);
		 g_ota_service_handle = NULL;
	}

	SEMPHR_TRY_UNLOCK(g_lock_ota);
	SEMPHR_DELETE_LOCK(g_lock_ota);
	g_lock_ota = NULL;
}

OTA_SERVICE_T *ota_service_create()
{
    OTA_SERVICE_T *ota = (OTA_SERVICE_T *) esp32_malloc(sizeof(OTA_SERVICE_T));
    ESP_ERROR_CHECK(!ota);
	memset(ota, 0, sizeof(OTA_SERVICE_T));
	g_ota_service_handle = ota;
	
    ota->ota_queue = xQueueCreate(1, sizeof(uint32_t));
	ota->ota_tone_queue = xQueueCreate(1, sizeof(uint32_t));
	SEMPHR_CREATE_LOCK(g_lock_ota);
	configASSERT(g_lock_ota);
    configASSERT(ota->ota_queue);
	configASSERT(ota->ota_tone_queue);
	
    ota->Based.deviceEvtNotified = DeviceEvtNotifiedToOTA;
    ota->Based.playerStatusUpdated = NULL;
    ota->Based.serviceActive = OtaServiceActive;
    ota->Based.serviceDeactive = OtaServiceDeactive;

    return ota;
}

void ota_service_send_msg(MSG_OTA_SERVICE_T _msg)
{
	SEMPHR_TRY_LOCK(g_lock_ota);
	if (g_ota_service_handle != NULL && g_ota_service_handle->ota_queue != NULL)
	{
		xQueueSend(g_ota_service_handle->ota_queue, &_msg, 0);
	}
	SEMPHR_TRY_UNLOCK(g_lock_ota);
}

OTA_UPGRADE_STATE get_ota_upgrade_state()
{
	OTA_UPGRADE_STATE state = OTA_UPGRADE_STATE_NOT_UPGRADING;
	SEMPHR_TRY_LOCK(g_lock_ota);
	if (g_ota_service_handle != NULL)
	{
		state = g_ota_service_handle->ota_state;
	} 
	SEMPHR_TRY_UNLOCK(g_lock_ota);
	return state;
}
