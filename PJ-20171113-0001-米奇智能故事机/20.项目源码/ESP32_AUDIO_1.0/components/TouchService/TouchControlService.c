#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#include "esp_log.h"
#include "driver/touch_pad.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#ifndef CONFIG_CLASSIC_BT_ENABLED
#include "esp_wifi.h"

#else
#include "esp_a2dp_api.h"

#endif
#if (defined CONFIG_ENABLE_TURINGAIWIFI_SERVICE || defined CONFIG_ENABLE_TURINGWECHAT_SERVICE)
#include "TuringRobotService.h"
#endif
#include "TouchControlService.h"
#include "touchpad.h"
#include "DeviceCommon.h"
#include "MediaHal.h"
#include "toneuri.h"
#include "EspAudioAlloc.h"
#include "EspAudio.h"
#include "userconfig.h"

#if (defined CONFIG_ENABLE_SMART_CONFIG) && (defined CONFIG_ENABLE_BLE_CONFIG)
#error "Can not enbale both smart config and ble config, please choose one of them !"
#endif

#define TOUCH_SER_TAG                   "TOUCH_CTRL_SERV"
#define TOUCH_SERV_TASK_PRIORITY        4
#define TOUCH_SERV_TASK_STACK_SIZE      2700

#define TOUCH_VOLUME_STEP               3

static xQueueHandle xQueueTouchService;
static TaskHandle_t TouchCtrlHandle;
static int HighWaterLine = 0;

#define URI_REC "i2s://16000:1@record.wav#file"
#define URI_REC_PLAYBACK "file:///sdcard/__record/record.wav"

esp_err_t SwitchBootPartition(void);

static void DeviceEvtNotifiedToTouch(DeviceNotification* note)
{
    // ESP_LOGI(TERM_TAG, "Service update, call from MediaControl\r\n");
    TouchControlService* service = (TouchControlService*) note->receiver;
    DeviceNotifyMsg msg = *((DeviceNotifyMsg*) note->data);

    if (note->type == DEVICE_NOTIFY_TYPE_TOUCH) {
        xQueueSend(xQueueTouchService, note->data, 0);
    } else if (note->type == DEVICE_NOTIFY_TYPE_AUXIN) {
        switch (msg) {
            case DEVICE_NOTIFY_AUXIN_INSERTED:
                service->Based._blocking = 1;
                break;

            case DEVICE_NOTIFY_AUXIN_REMOVED:
                service->Based._blocking = 0;
                break;

            default:
                break;
        }
    } else if (note->type == DEVICE_NOTIFY_TYPE_WIFI) {
        switch (msg) {
            case DEVICE_NOTIFY_WIFI_GOT_IP:
                service->_smartCfg = 0;

                if (service->_bleCfg == 1) {
                    service->_bleCfg = 0;
                    service->Based.sendEvent((MediaService*) service, MEDIA_WIFI_BLE_CONFIG_STOP, NULL, 0);
                }

                service->Based.mediaResume((MediaService*) service);
                break;

            default:
                break;
        }
    }
}

static void PlayerStatusUpdatedToTouch(ServiceEvent* event)
{

}

void touchServiceTask(void* pv)
{
    int touchEvent;
    BaseType_t xStatus;
    TouchControlService* service = (TouchControlService*) pv;
    ESP_LOGI(TOUCH_SER_TAG, "Touch Control Service running");
    static int recodingFlg = 0;
    int _playmode;

    while (1) {
        xStatus = xQueueReceive(xQueueTouchService, &touchEvent, portMAX_DELAY);

        if (xStatus == pdPASS) {
            switch (touchEvent) {
                case DEVICE_NOTIFY_KEY_VOL_UP: {
                    ESP_LOGI(TOUCH_SER_TAG, "vol up");

                    if (service->Based.setVolume) {
                        if (service->Based.setVolume((MediaService*) service, 1, TOUCH_VOLUME_STEP) < 0) {
                            ESP_LOGE(TOUCH_SER_TAG, "Cannot set volume now");
                        }
                    }
                }
                break;

                case DEVICE_NOTIFY_KEY_VOL_DOWN: {
                    ESP_LOGI(TOUCH_SER_TAG, "vol down");

                    if (service->Based.setVolume) {
                        if (service->Based.setVolume((MediaService*) service, 1, 0 - TOUCH_VOLUME_STEP) < 0) {
                            ESP_LOGE(TOUCH_SER_TAG, "Cannot set volume now");
                        }
                    }
                }
                break;

                case DEVICE_NOTIFY_KEY_BT_WIFI_SWITCH: {
                    if (!service->Based._blocking) {
                        if (SwitchBootPartition() == ESP_OK) {
#ifndef CONFIG_CLASSIC_BT_ENABLED
                            if(esp_wifi_stop() != 0) {
                                ESP_LOGE(TOUCH_SER_TAG, "esp_wifi_stop failed");
                                SwitchBootPartition();
                                return;
                            }
#endif
#ifdef CONFIG_CLASSIC_BT_ENABLED
                            if(esp_a2d_sink_disconnect(NULL) != 0) {
                                ESP_LOGE(TOUCH_SER_TAG, "esp_a2d_sink_disconnect failed");
                                SwitchBootPartition();
                                return;
                            }
#endif
                            MediaHalPaPwr(0);
                            esp_restart();
                        }

                    }
                }
                break;
#if (defined CONFIG_ENABLE_TURINGAIWIFI_SERVICE && defined CONFIG_ENABLE_TURINGWECHAT_SERVICE)
                case DEVICE_NOTIFY_KEY_AIWIFI_WECHAT_SWITCH: {
                    EspAudioVolSet(60);
                    if(get_current_turing_mode() == AIWIFI_MODE) {
                        service->Based.playTone((MediaService*)service, tone_uri[TONE_TYPE_WECHAT], TERMINATION_TYPE_NOW);
                        turing_switch_to(WECHAT_MODE);
                    } else if(get_current_turing_mode() == WECHAT_MODE || get_current_turing_mode() == NULL_MODE ){
                        service->Based.playTone((MediaService*)service, tone_uri[TONE_TYPE_FREETALK], TERMINATION_TYPE_NOW);
                        turing_switch_to(AIWIFI_MODE);
                    }
                   }
                   break;
#endif
#ifndef CONFIG_CLASSIC_BT_ENABLED

                case DEVICE_NOTIFY_KEY_NEXT: {
                    if (!service->Based._blocking) {
                        if (service->Based.mediaNext) {
                            service->Based.mediaNext((MediaService*) service);
                        }

                        if (service->Based.mediaPlay) {
                            service->Based.mediaPlay((MediaService*) service);
                        }
                    }
                }
                break;

                case DEVICE_NOTIFY_KEY_PREV: {
                    if (!service->Based._blocking) {
                        if (service->Based.mediaPrev) {
                            service->Based.mediaPrev((MediaService*) service);
                        }

                        if (service->Based.mediaPlay) {
                            service->Based.mediaPlay((MediaService*) service);
                        }
                    }
                }
                break;

                case DEVICE_NOTIFY_KEY_PLAY: {
                    if (!service->Based._blocking) {
                        PlayerStatus player = {0};
                        service->Based.getPlayerStatus((MediaService*)service, &player);

                        if (player.status != AUDIO_STATUS_PLAYING) {
                            if (player.status == AUDIO_STATUS_PAUSED) {
                                service->Based.mediaResume((MediaService*) service);
                            } else {
                                if (EspAudioPlay(NULL) == AUDIO_ERR_PLAYER_NO_AUDIO_AVAILABLE) {
                                    EspAudioNext();
                                    EspAudioPlay(NULL);
                                }
                            }
                        } else {
                            service->Based.mediaPause((MediaService*) service);
                        }
                    }
                }
                break;

                case DEVICE_NOTIFY_KEY_WIFI_SET:
                    if (!service->Based._blocking) {
                        PlayerStatus player = {0};
                        service->Based.getPlayerStatus((MediaService*)service, &player);

                        if (AUDIO_STATUS_PLAYING == player.status) {
                            service->Based.mediaPause((MediaService*)service);
                        }

#ifdef  CONFIG_ENABLE_SMART_CONFIG
                        service->_smartCfg = 1;
                        service->Based.sendEvent((MediaService*) service, MEDIA_WIFI_SMART_CONFIG_REQUEST, NULL, 0);
#elif (defined CONFIG_ENABLE_BLE_CONFIG)
                        service->_bleCfg = 1;
                        service->Based.sendEvent((MediaService*) service, MEDIA_WIFI_BLE_CONFIG_REQUEST, NULL, 0);
#endif

                        if (service->Based.playTone) {
                            //service->Based.playTone(service, tone_uri[TONE_TYPE_UNDER_SMARTCONFIG], TERMINATION_TYPE_NOW);
                        }
                    }

                    break;

                case DEVICE_NOTIFY_KEY_REC: {
#if CONFIG_ENABLE_RECOGNITION_SERVICE == 0
#if (!defined CONFIG_ENABLE_TURINGAIWIFI_SERVICE && !defined CONFIG_ENABLE_TURINGWECHAT_SERVICE)

                    if (!service->Based._blocking) {
                        service->Based.rawStart((MediaService*)service, URI_REC);
                        recodingFlg = 1;
                    }
#endif
#endif
                }
                break;

                case DEVICE_NOTIFY_KEY_REC_QUIT: {
#if CONFIG_ENABLE_RECOGNITION_SERVICE == 0
#if (!defined CONFIG_ENABLE_TURINGAIWIFI_SERVICE && !defined CONFIG_ENABLE_TURINGWECHAT_SERVICE)
                    if (!service->Based._blocking) {
                        if (recodingFlg) {
                            service->Based.rawStop((MediaService*)service, TERMINATION_TYPE_DONE);
                            service->Based.addUri((MediaService*) service, URI_REC_PLAYBACK);
                            service->Based.mediaPlay((MediaService*) service);
                            recodingFlg = 0;
                        }
                    }
#endif
#endif
                }
                break;

                case DEVICE_NOTIFY_KEY_MODE: {
                    if (!service->Based._blocking) {
                        service->Based.getPlayMode((MediaService*)service, &_playmode);
                        _playmode++;

                        if (_playmode > MEDIA_PLAY_REPEAT) { // mean to skip MEDIA_PLAY_ONE_SONG
                            _playmode = 0;
                        }

                        service->Based.setPlayMode((MediaService*)service, _playmode);
                        ESP_LOGI(TOUCH_SER_TAG, "Play mode: %s", _playmode == MEDIA_PLAY_SEQUENTIAL ? "Sequential" :
                                 (_playmode == MEDIA_PLAY_SHUFFLE ? "Shuffle" : "Repeat"));
                    }
                }
                break;

#endif

                default:
                    ESP_LOGI(TOUCH_SER_TAG, "Not supported line=%d", __LINE__);
                    break;

            }
        }

#if EN_STACK_TRACKER

        if (uxTaskGetStackHighWaterMark(TouchCtrlHandle) > HighWaterLine) {
            HighWaterLine = uxTaskGetStackHighWaterMark(TouchCtrlHandle);
            ESP_LOGI("STACK", "%s %d\n\n\n", __func__, HighWaterLine);
        }

#endif
    }

    vTaskDelete(NULL);
}


static void TouchControlActive(MediaService* self)
{
    TouchControlService* service = (TouchControlService*) self;

    if (xTaskCreate(touchServiceTask,
                    "touchServiceTask",
                    TOUCH_SERV_TASK_STACK_SIZE,
                    service,
                    TOUCH_SERV_TASK_PRIORITY,
                    &TouchCtrlHandle) != pdPASS) {

        ESP_LOGE(TOUCH_SER_TAG, "ERROR creating touchServiceTask task! Out of memory?");
    }

    ESP_LOGI(TOUCH_SER_TAG, "touchControlActive\r\n");
}

static void TouchControlDeactive(MediaService* self)
{
//    TouchControlService *service = (TouchControlService *) self;
    ESP_LOGI(TOUCH_SER_TAG, "touchControlDeactive\r\n");
}

TouchControlService* TouchControlCreate()
{
    TouchControlService* touch = (TouchControlService*) EspAudioAlloc(1, sizeof(TouchControlService));
    ESP_ERROR_CHECK(!touch);

    touch->Based.deviceEvtNotified = DeviceEvtNotifiedToTouch;
    touch->Based.playerStatusUpdated = PlayerStatusUpdatedToTouch;
    touch->Based.serviceActive = TouchControlActive;
    touch->Based.serviceDeactive = TouchControlDeactive;
    touch->_smartCfg = 0;
    touch->_bleCfg = 0;
    xQueueTouchService = xQueueCreate(5, sizeof(int));
    configASSERT(xQueueTouchService);
    return touch;
}
