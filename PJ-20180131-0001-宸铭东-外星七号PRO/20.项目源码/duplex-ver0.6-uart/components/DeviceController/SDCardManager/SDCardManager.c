#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "DeviceManager.h"
#include "SDCardManager.h"
#include "NotifyHelper.h"
#include "DeviceCommon.h"

#include "SDCardConfig.h"
#include "SDCardDef.h"
#include "userconfig.h"
#include "sd_music_manage.h"

#define SDCARD_MANAGER_TAG                      "SDCARD_MANAGER"

#define SDCARD_EVT_QUEUE_LEN                    1
#define SDCARD_DETECTION_TASK_PRIORITY          4
#define SDCARD_DETECTION_TASK_STACK_SIZE        (2048 + 512)

static xQueueHandle sdcardEvtQueue;
static SDCardManager *sSdCardManager;

static int HighWaterLine = 0;
TaskHandle_t SdManagerHandle;
extern int TfCardMounting;

static void SDCardDetectionTask(void *pvParameters)
{
    SDCardManager *manager = (SDCardManager *) pvParameters;
    int trigger;
    char lastState = 0;
    int status = sd_card_status_detect();
    if (status == 0) {
        lastState = 1;
    }
    while (1) {
        if (xQueuePeek(sdcardEvtQueue, &trigger, portMAX_DELAY)) {
            esp_err_t ret;
            DeviceNotifySDCardMsg msg = DEVICE_NOTIFY_SD_UNDEFINED;
            /*XXX: currently a sudden insertion/removal of sd card while playing
             * would cause problem, so pause media first. May not need to if the
             * problem can be solved
             */
			#if 0
            if (manager->Based.pauseMedia) {
                manager->Based.pauseMedia((DeviceManager *) manager);
            }
			#endif
            if (lastState == 1) {
                ESP_LOGI(SDCARD_MANAGER_TAG, "unmount");
                while(TfCardMounting == 1) {
                    ESP_LOGI(SDCARD_MANAGER_TAG, "waiting mount done");
                    vTaskDelay(1000/portTICK_RATE_MS);
                }
                ret = sd_card_unmount();
                if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE) {
                    lastState = 0;
                    msg = DEVICE_NOTIFY_SD_UNMOUNTED;
					sd_music_manage_unmount();
                } else {
                    ESP_LOGE(SDCARD_MANAGER_TAG, "unmount failed");
                }
            }
            status = sd_card_status_detect();
            if (status == 0 && lastState == 0) {  //card inserted && didn't mounted
                ESP_LOGI(SDCARD_MANAGER_TAG, "mount");
                ret = sd_card_mount(SD_CARD_DEFAULT_BASE_PATH);
                if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE) {
                    lastState = 1;
                    msg = DEVICE_NOTIFY_SD_MOUNTED;
					sd_music_manage_mount();
                } else {
                    lastState = 0;
                    msg = DEVICE_NOTIFY_SD_MOUNT_FAILED;
                }
            }
            manager->Based.notify((struct DeviceController *) manager->Based.controller, DEVICE_NOTIFY_TYPE_SDCARD, &msg, sizeof(DeviceNotifySDCardMsg));
            xQueueReceive(sdcardEvtQueue, &trigger, portMAX_DELAY);
#if EN_STACK_TRACKER
            if(uxTaskGetStackHighWaterMark(SdManagerHandle) > HighWaterLine){
                HighWaterLine = uxTaskGetStackHighWaterMark(SdManagerHandle);
                ESP_LOGI("STACK", "%s %d\n\n\n", __func__, HighWaterLine);
            }
#endif
        }
    }
    vTaskDelete(NULL);
}

static int SDCardManagerActive(DeviceManager *self)
{
    SDCardManager *manager = (SDCardManager *) self;
    esp_err_t ret = 0;

#if WROAR_BOARD
    sdcardEvtQueue = xQueueCreate(SDCARD_EVT_QUEUE_LEN, sizeof(int));
    configASSERT(sdcardEvtQueue);
    sd_card_intr_init(sdcardEvtQueue);  //enable interrupt
    if (sd_card_status_detect() == 0) {  //card is inserted
#endif
        DeviceNotifySDCardMsg msg;
        ret = sd_card_mount(SD_CARD_DEFAULT_BASE_PATH);
        if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE) {
            msg = DEVICE_NOTIFY_SD_MOUNTED;
        } else {
            ret = sd_card_unmount();
            if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE) {
                msg = DEVICE_NOTIFY_SD_UNMOUNTED;
            }
        }
        manager->Based.notify((struct DeviceController *) manager->Based.controller, DEVICE_NOTIFY_TYPE_SDCARD, &msg, sizeof(DeviceNotifySDCardMsg));
#if WROAR_BOARD
    }
    if (xTaskCreatePinnedToCore(SDCardDetectionTask,
                                "SDCardDetectionTask",
                                SDCARD_DETECTION_TASK_STACK_SIZE,
                                manager,
                                SDCARD_DETECTION_TASK_PRIORITY,
                                &SdManagerHandle, 0) != pdPASS) {

        ESP_LOGE(SDCARD_MANAGER_TAG, "ERROR creating SDCardDetectionTask! Out of memory?");
        return -1;
    }
#endif
    return ret;

}

static void SDCardManagerDeactive(DeviceManager *self)
{
    //TODO
}

SDCardManager *SDCardManagerCreate(struct DeviceController *controller)
{
    if (!controller)
        return NULL;
    if (sSdCardManager) {
        return sSdCardManager;
    }
    ESP_LOGI(SDCARD_MANAGER_TAG, "SDCardManager Create\r\n");
   sSdCardManager = (SDCardManager *) calloc(1, sizeof(SDCardManager));
    ESP_ERROR_CHECK(!sSdCardManager);
    InitManager((DeviceManager *) sSdCardManager, controller);
    sSdCardManager->Based.active = SDCardManagerActive;
    sSdCardManager->Based.deactive = SDCardManagerDeactive;
    sSdCardManager->Based.notify = SDCardEvtNotify;  //FIXME: conflict signature
    return sSdCardManager;
}
