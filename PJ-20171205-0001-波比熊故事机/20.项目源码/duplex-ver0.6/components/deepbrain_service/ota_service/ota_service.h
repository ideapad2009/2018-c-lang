#ifndef __OTA_SERVICE_H__
#define __OTA_SERVICE_H__
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "MediaService.h"

typedef enum
{
	OTA_UPGRADE_STATE_UPGRADING = 0,
	OTA_UPGRADE_STATE_NOT_UPGRADING,
}OTA_UPGRADE_STATE;

typedef enum
{
	OTA_PLAY_TONE_MSG_NONE,
	OTA_PLAY_TONE_MSG_START,
	OTA_PLAY_TONE_MSG_STOP,
}OTA_PLAY_TONE_MSG_T;

struct AudioTrack;

extern QueueHandle_t g_ota_queue;
extern OTA_UPGRADE_STATE g_ota_upgrade_state;

typedef struct
{
    /*relation*/
    MediaService Based;
    struct AudioTrack *_receivedTrack;  //FIXME: is this needed??
} OTA_SERVICE_T;

typedef enum
{
	MSG_OTA_IDEL,
	MSG_OTA_SD_UPDATE,
	MSG_OTA_WIFI_UPDATE
}MSG_OTA_SERVICE_T;

OTA_SERVICE_T *ota_service_create(void);
void ota_service_send_msg(MSG_OTA_SERVICE_T _msg);
OTA_UPGRADE_STATE get_ota_upgrade_state();

#endif
