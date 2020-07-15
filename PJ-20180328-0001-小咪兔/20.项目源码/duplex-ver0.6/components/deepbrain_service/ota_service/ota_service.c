#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "ota_update.h"
#include "MediaService.h"
#include "ota_service.h"
#include "DeviceCommon.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
//#include "keyboard_matrix.h"
#include "EspAudio.h"
#include "toneuri.h"
#include "MediaHal.h"
#include "flash_config_manage.h"
#include "player_middleware.h"
#include "auto_play_service.h"

#define OTA_SERV_TAG "OTA_SERVICE"
#define OTA_SERVICE_TASK_PRIORITY               13
#define OTA_SERVICE_TASK_STACK_SIZE             7*1024

QueueHandle_t g_ota_queue = NULL;
xTimerHandle g_sth = NULL;
static int g_stop_ota = 0;
OTA_UPGRADE_STATE g_ota_upgrade_state = OTA_UPGRADE_STATE_NOT_UPGRADING;

void upgrading_play_music()
{	
	if (g_sth != NULL && g_stop_ota == 0)
	{
		auto_play_stop();
		player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);
		xTimerStart(g_sth, 50/portTICK_PERIOD_MS);
	}
	else
	{
		xTimerStop(g_sth, 50/portTICK_PERIOD_MS);
		xTimerDelete(g_sth, 50/portTICK_PERIOD_MS);
		g_sth = NULL;
	}
}

void upgrading_play_stop()
{		
	g_stop_ota = 1;
}

void upgrading_play_music_init()
{
	g_stop_ota = 0;
	g_sth = xTimerCreate((const char*)"upgrading_play_music", 1000/portTICK_PERIOD_MS, pdFALSE, &g_sth,
                upgrading_play_music);
	xTimerStart(g_sth, 50/portTICK_PERIOD_MS);
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
				memset(str_fw_url, 0, sizeof(str_fw_url));
				if (ota_update_check_fw_version("http://file.yuyiyun.com:2088/ota/PJ-20171113-0001/version.txt", str_fw_url, sizeof(str_fw_url)) == OTA_NONEED_UPDATE_FW)
				{
					break;
				}
				g_ota_upgrade_state = OTA_UPGRADE_STATE_UPGRADING;
				auto_play_stop();
				vTaskDelay(3000);	//连网成功语音缓冲时间
				player_mdware_play_tone(FLASH_MUSIC_EQUIPMENT_UPGRADING);
				vTaskDelay(5000);
				ESP_LOGI(OTA_SERV_TAG, "ota_update_from_wifi begin");
				upgrading_play_music_init();
				if (ota_update_from_wifi(str_fw_url) == OTA_SUCCESS)
				{
					upgrading_play_stop();
					vTaskDelay(1000);	//停止后音效缓冲时间
					player_mdware_play_tone(FLASH_MUSIC_UPGRADE_COMPLETE);
					vTaskDelay(3000);
					MediaHalPaPwr(0);
					esp_restart();
				}
				else
				{
					upgrading_play_stop();
					vTaskDelay(1000);	//停止后音效缓冲时间
					player_mdware_play_tone(FLASH_MUSIC_UPDATE_FAIL);
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
                                OTA_SERVICE_TASK_STACK_SIZE,
                                service,
                                OTA_SERVICE_TASK_PRIORITY,
                                NULL, xPortGetCoreID()) != pdPASS) {
        ESP_LOGE(OTA_SERV_TAG, "Error create OtaServiceTask");
    }
}

static void OtaServiceDeactive(OTA_SERVICE_T *service)
{
    ESP_LOGI(OTA_SERV_TAG, "OtaServiceStop");
    vQueueDelete(g_ota_queue);
    g_ota_queue = NULL;
}

OTA_SERVICE_T *ota_service_create()
{
    OTA_SERVICE_T *ota = (OTA_SERVICE_T *) calloc(1, sizeof(OTA_SERVICE_T));
    ESP_ERROR_CHECK(!ota);
    g_ota_queue = xQueueCreate(2, sizeof(uint32_t));
    configASSERT(g_ota_queue);
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
