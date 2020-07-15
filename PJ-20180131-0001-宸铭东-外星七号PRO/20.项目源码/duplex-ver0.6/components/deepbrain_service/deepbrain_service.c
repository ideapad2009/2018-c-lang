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
#include "led_ctrl.h"
#include "nlp_service.h"
#include "keyboard_service.h"
#include "wechat_service.h"
#include "bind_device.h"
#include "auto_play_service.h"
#include "player_middleware.h"
#include "sd_music_manage.h"
#include "low_power_manage.h"
#include "memo_service.h"
#include "geling_edu.h"
#include "translate_manage.h"
#if MODULE_FREE_TALK
#include "free_talk.h"
#endif
#if MODULE_KEYWORD_WAKEUP
#include "keyword_wakeup.h"
#endif

#define DEEPBRAIN_SER_TAG                   "DEEP_BRAIN_SERV"
#define DEEPBRAIN_SERV_TASK_PRIORITY        4

DeepBrainService *g_deepbrain_service = NULL;

typedef enum
{
	AUDIO_RECORDING,
	AUDIO_END
}ENUM_AUDIO_STATUS;

static void DeviceEvtNotifiedToWifi(DeviceNotification *note)
{
    DeepBrainService *service = (DeepBrainService *) note->receiver;
    DeviceNotifyMsg msg = *((DeviceNotifyMsg *) note->data);
    if (note->type == DEVICE_NOTIFY_TYPE_WIFI) 
	{
        switch (msg) 
		{
        case DEVICE_NOTIFY_WIFI_GOT_IP:
            service->Based._blocking = 0;
			DEBUG_LOGE(DEEPBRAIN_SER_TAG, "DEVICE_NOTIFY_WIFI_GOT_IP");
            break;
        case DEVICE_NOTIFY_WIFI_DISCONNECTED:
            service->Based._blocking = 1;
			DEBUG_LOGE(DEEPBRAIN_SER_TAG, "DEVICE_NOTIFY_WIFI_DISCONNECTED");
            break;
		case DEVICE_NOTIFY_WIFI_SETTING_TIMEOUT:
            service->Based._blocking = 1;
			DEBUG_LOGE(DEEPBRAIN_SER_TAG, "DEVICE_NOTIFY_WIFI_SETTING_TIMEOUT");
            break;
		case DEVICE_NOTIFY_WIFI_SC_DISCONNECTED:
            service->Based._blocking = 1;
			DEBUG_LOGE(DEEPBRAIN_SER_TAG, "DEVICE_NOTIFY_WIFI_SC_DISCONNECTED");
            break;
        default:
            break;
        }
    }
}

static void PlayerStatusUpdatedToTouch(ServiceEvent *event)
{
	switch(event->type)
	{
		case PLAYER_CHANGE_STATUS:
		{
			DEBUG_LOGE(DEEPBRAIN_SER_TAG, "PLAYER_CHANGE_STATUS");
			break;
		}
		case PLAYER_ENCODE_DATA:
		{
			DEBUG_LOGE(DEEPBRAIN_SER_TAG, "PLAYER_ENCODE_DATA");
			break;
		}
		case PLAYER_DECODE_DATA:
		{
			DEBUG_LOGE(DEEPBRAIN_SER_TAG, "PLAYER_DECODE_DATA");
			break;
		}
		case MEDIA_WIFI_SMART_CONFIG_REQUEST:
		{
			DEBUG_LOGE(DEEPBRAIN_SER_TAG, "MEDIA_WIFI_SMART_CONFIG_REQUEST");
			break;
		}
		default:
			break;
	}
}

static void DeepBrainActive(MediaService *self)
{
    DeepBrainService *service = (DeepBrainService *) self;

#if MODULE_KEYWORD_WAKEUP
	keyword_wakeup_create(service);
#endif
#if MODULE_FREE_TALK
	free_talk_create(service);
#endif
	asr_service_create();
	nlp_service_create();
	keyboard_service_create(service);
	wechat_service_create();
	bind_device_create();
	auto_play_service_create();
	sd_music_manage_create();
	memo_service_create(service);
	low_power_manage_create();
	geling_edu_service_create();
	
    DEBUG_LOGI(DEEPBRAIN_SER_TAG, "DeepBrainActive\r\n");
}

static void DeepBrainDeactive(MediaService *self)
{
    DeepBrainService *service = (DeepBrainService *) self;

	nlp_service_delete();
	keyboard_service_delete();
#if MODULE_FREE_TALK
	free_talk_delete();
#endif
	wechat_service_delete();
	bind_device_delete();
	auto_play_service_delete();
	sd_music_manage_delete();
	geling_edu_service_delete();
	
	esp32_free(service);
    DEBUG_LOGI(DEEPBRAIN_SER_TAG, "DeepBrainDeactive\r\n");
}

DeepBrainService *DeepBrainCreate()
{
    DeepBrainService *deepBrain = (DeepBrainService *)esp32_malloc(sizeof(DeepBrainService));
    ESP_ERROR_CHECK(!deepBrain);
	g_deepbrain_service = deepBrain;
	
    deepBrain->Based.deviceEvtNotified = DeviceEvtNotifiedToWifi;
    deepBrain->Based.playerStatusUpdated = PlayerStatusUpdatedToTouch;
    deepBrain->Based.serviceActive = DeepBrainActive;
    deepBrain->Based.serviceDeactive = DeepBrainDeactive;
	deepBrain->Based._blocking = 1;

    return deepBrain;
}
