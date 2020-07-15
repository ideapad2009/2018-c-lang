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
#include "free_talk.h"
#include "EspAudio.h"
#include "toneuri.h"
#include "MediaHal.h"
#include "flash_config_manage.h"
#include "player_middleware.h"
#include "led_ctrl.h"
#include "auto_play_service.h"

#define OTA_SERV_TAG "OTA_SERVICE"

QueueHandle_t g_ota_queue = NULL;				
QueueHandle_t g_ota_tone_queue = NULL;			
static int g_stop_ota = 0;
OTA_UPGRADE_STATE g_ota_upgrade_state = OTA_UPGRADE_STATE_NOT_UPGRADING;

static void ota_service_play_tone_start(void)
{
	auto_play_stop();
	OTA_PLAY_TONE_MSG_T msg = OTA_PLAY_TONE_MSG_START;
	
	if (g_ota_tone_queue != NULL)
	{
		xQueueSend(g_ota_tone_queue, &msg, 0);
	}
}

static void ota_service_play_tone_stop(void)
{
	OTA_PLAY_TONE_MSG_T msg = OTA_PLAY_TONE_MSG_STOP;
	
	if (g_ota_tone_queue != NULL)
	{
		xQueueSend(g_ota_tone_queue, &msg, 0);
	}
}

static void ota_service_play_tone(void *pvParameters)
{
	OTA_PLAY_TONE_MSG_T msg = OTA_PLAY_TONE_MSG_NONE;
	OTA_PLAY_TONE_MSG_T status = OTA_PLAY_TONE_MSG_NONE; 
	
    while (1) 
	{
		msg = OTA_PLAY_TONE_MSG_NONE;
        if (xQueueReceive(g_ota_tone_queue, &msg, (TickType_t)2000) == pdPASS)
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
				player_mdware_play_tone(FLASH_MUSIC_E_03_UPGRADING_MUSIC);	
				break;
			}
			case OTA_PLAY_TONE_MSG_STOP:
			{
				break;
			}
			default:
				break;
		}
		
    }
    vTaskDelete(NULL);
}

static void DeviceEvtNotifiedToOTA(DeviceNotification *note)
{
    OTA_SERVICE_T *service = (OTA_SERVICE_T *) note->receiver;
    if (DEVICE_NOTIFY_TYPE_WIFI == note->type) {
        DeviceNotifyMsg msg = *((DeviceNotifyMsg *) note->data);
        switch (msg) {
        case DEVICE_NOTIFY_WIFI_GOT_IP:
            ota_service_send_msg(MSG_OTA_WIFI_UPDATE);
            break;
        default:
            break;
        }
    }
}

static void PlayerStatusUpdatedToOTA(ServiceEvent *event)
{

}

static void OtaServiceTask(void *pvParameters)
{
    OTA_SERVICE_T *service  = (OTA_SERVICE_T *) pvParameters;
    ESP_LOGI(OTA_SERV_TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    MSG_OTA_SERVICE_T msg = MSG_OTA_IDEL;
	OTA_UPDATE_MODE_T ota_mode = OTA_UPDATE_MODE_FORMAL;
	char str_fw_url[128] = {0};
	vTaskDelay(10*1000);
    while (1) 
	{
		msg = MSG_OTA_IDEL;
        xQueueReceive(g_ota_queue, &msg, portMAX_DELAY);
		switch (msg)
		{
			case MSG_OTA_WIFI_UPDATE:
			{
//				led_ctrl_set_mode(LED_CTRL_EYE_LIGHT);
				memset(str_fw_url, 0, sizeof(str_fw_url));
				
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
				
				g_ota_upgrade_state = OTA_UPGRADE_STATE_UPGRADING;
				free_talk_stop();
				set_free_talk_pause();
				auto_play_stop();
				vTaskDelay(1000);	//连网成功语音缓冲时间
				player_mdware_play_tone(FLASH_MUSIC_B_23_NEW_VERSION_AVAILABLE);
				vTaskDelay(5000);
				ESP_LOGI(OTA_SERV_TAG, "ota_update_from_wifi begin");
				ota_service_play_tone_start();
				if (ota_update_from_wifi(str_fw_url) == OTA_SUCCESS)
				{
					ota_service_play_tone_stop();
					vTaskDelay(1000);	//停止后音效缓冲时间
					player_mdware_play_tone(FLASH_MUSIC_B_14_UPGRADE_SUCCESSFUL);
					vTaskDelay(3000);
					MediaHalPaPwr(0);
					esp_restart();
				}
				else
				{
					set_free_talk_resume();
					ota_service_play_tone_stop();
					vTaskDelay(1000);	//停止后音效缓冲时间
					player_mdware_play_tone(FLASH_MUSIC_B_15_UPGRADE_FAIL);
					vTaskDelay(3000);
					g_ota_upgrade_state = OTA_UPGRADE_STATE_NOT_UPGRADING;
					ESP_LOGE(OTA_SERV_TAG, "ota_update_from_wifi failed");
				}
				break;
			}
			default:
				break;
		}
    }
    vTaskDelete(NULL);
}

static void OtaServiceActive(OTA_SERVICE_T *service)
{
    ESP_LOGI(OTA_SERV_TAG, "OtaService active");
    if (xTaskCreatePinnedToCore(OtaServiceTask,
                    "OtaServiceTask",
                    6*1024,
                    service,
                    10,
                    NULL, xPortGetCoreID()) != pdPASS) 
    {
        ESP_LOGE(OTA_SERV_TAG, "Error create OtaServiceTask");
    }

	if (xTaskCreate(ota_service_play_tone,
                    "ota_service_play_tone",
                    3*1024,
                    service,
                    10,
                    NULL) != pdPASS) 
    {
        ESP_LOGE(OTA_SERV_TAG, "Error create ota_service_play_tone");
    }
}

static void OtaServiceDeactive(OTA_SERVICE_T *service)
{
    ESP_LOGI(OTA_SERV_TAG, "OtaServiceStop");
    vQueueDelete(g_ota_queue);
    g_ota_queue = NULL;
	vQueueDelete(g_ota_tone_queue);
    g_ota_tone_queue = NULL;
}

OTA_SERVICE_T *ota_service_create()
{
    OTA_SERVICE_T *ota = (OTA_SERVICE_T *) calloc(1, sizeof(OTA_SERVICE_T));
    ESP_ERROR_CHECK(!ota);
    g_ota_queue = xQueueCreate(5, sizeof(uint32_t));
	g_ota_tone_queue = xQueueCreate(1, sizeof(uint32_t));
    configASSERT(g_ota_queue);
	configASSERT(g_ota_tone_queue);
    ota->Based.deviceEvtNotified = DeviceEvtNotifiedToOTA;
    ota->Based.playerStatusUpdated = PlayerStatusUpdatedToOTA;
    ota->Based.serviceActive = OtaServiceActive;
    ota->Based.serviceDeactive = OtaServiceDeactive;

    return ota;
}

void ota_service_send_msg(MSG_OTA_SERVICE_T _msg)
{
	if (g_ota_queue != NULL)
	{
		xQueueSend(g_ota_queue, &_msg, 0);
	}
}

OTA_UPGRADE_STATE get_ota_upgrade_state()
{
	OTA_UPGRADE_STATE ota_upgrade_state = g_ota_upgrade_state;
	return ota_upgrade_state;
}
