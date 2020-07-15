#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "MediaService.h"
#include "ToneOtaService.h"
#include "DeviceCommon.h"

#define TONE_OTA_SERV_TAG "TONE_OTA_SERVICE"
#define TONE_OTA_SERVICE_TASK_PRIORITY               13
#define TONE_OTA_SERVICE_TASK_STACK_SIZE             3*1024

static void DeviceEvtNotifiedToToneOTA(DeviceNotification* note)
{
    ToneOtaService* service = (ToneOtaService*) note->receiver;

    if (DEVICE_NOTIFY_TYPE_WIFI == note->type) {
        DeviceNotifyMsg msg = *((DeviceNotifyMsg*) note->data);

        switch (msg) {
            case DEVICE_NOTIFY_WIFI_GOT_IP:
                xQueueSend(service->_ToneOtaQueue, &msg, 0);
                break;

            case DEVICE_NOTIFY_WIFI_DISCONNECTED:
                xQueueSend(service->_ToneOtaQueue, &msg, 0);
                break;

            default:
                break;
        }
    }
}

static void ToneOtaServiceTask(void* pvParameters)
{
    ToneOtaService* service  = (ToneOtaService*) pvParameters;
    ESP_LOGI(TONE_OTA_SERV_TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    uint32_t wifiStatus = 0;
    uint32_t ToneOtaStatus = 0;

    while (1) {
        xQueueReceive(service->_ToneOtaQueue, &wifiStatus, portMAX_DELAY);

        if (wifiStatus == DEVICE_NOTIFY_WIFI_GOT_IP) {
            ESP_LOGI(TONE_OTA_SERV_TAG, "Wifi has been connected !");
            ToneOtaStatus = esp_tone_ota_audio_get_status();

            if (ToneOtaStatus == OTA_STATUS_INIT) {
                ESP_LOGW(TONE_OTA_SERV_TAG, "tone ota status partition in init");
                esp_tone_ota_audio_set_status(OTA_STATUS_IDLE | OTA_RES_SUCCESS);
            }

            if (esp_tone_ota_audio_begin(TONE_OTA_URL) != ESP_OK) {
                ToneOtaStatus = esp_tone_ota_audio_get_status();
                ESP_LOGE(TONE_OTA_SERV_TAG, "tone ota failed in status 0x%08x state 0x%08x", (ToneOtaStatus & 0xFFFF0000), (ToneOtaStatus & 0x0000FFFF))
            } else {
                ESP_LOGI(TONE_OTA_SERV_TAG, "download flash tone done");
                esp_tone_ota_audio_set_status(OTA_STATUS_END | OTA_RES_PROGRESS);

                if (ESP_OK == esp_tone_ota_audio_end()) {
                    ESP_LOGI(TONE_OTA_SERV_TAG, "end tone partition ok");
                } else {
                    ESP_LOGE(TONE_OTA_SERV_TAG, "end tone partition check failed");
                }
            }

            wifiStatus = 0;
        }
    }

    vTaskDelete(NULL);
}

static void ToneOtaServiceActive(ToneOtaService* service)
{
    ESP_LOGI(TONE_OTA_SERV_TAG, "ToneOtaService active");

    if (xTaskCreatePinnedToCore(ToneOtaServiceTask,
                                "ToneOtaServiceTask",
                                TONE_OTA_SERVICE_TASK_STACK_SIZE,
                                service,
                                TONE_OTA_SERVICE_TASK_PRIORITY,
                                NULL, xPortGetCoreID()) != pdPASS) {
        ESP_LOGE(TONE_OTA_SERV_TAG, "Error create ToneOtaServiceTask");
    }
}

static void ToneOtaServiceDeactive(ToneOtaService* service)
{
    ESP_LOGI(TONE_OTA_SERV_TAG, "ToneOtaServiceStop");
    vQueueDelete(service->_ToneOtaQueue);
    service->_ToneOtaQueue = NULL;
}

ToneOtaService* ToneOtaServiceCreate()
{
    ToneOtaService* ota = (ToneOtaService*) calloc(1, sizeof(ToneOtaService));
    ESP_ERROR_CHECK(!ota);
    ota->_ToneOtaQueue = xQueueCreate(1, sizeof(uint32_t));
    configASSERT(ota->_ToneOtaQueue);
    ota->Based.deviceEvtNotified = DeviceEvtNotifiedToToneOTA;
    ota->Based.serviceActive = ToneOtaServiceActive;

    return ota;
}
