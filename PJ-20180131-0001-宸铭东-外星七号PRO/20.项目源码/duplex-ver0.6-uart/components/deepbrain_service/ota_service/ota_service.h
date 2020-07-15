#ifndef __OTA_SERVICE_H__
#define __OTA_SERVICE_H__
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "MediaService.h"

/*ota 升级状态*/
typedef enum
{
	OTA_UPGRADE_STATE_NOT_UPGRADING = 0,
	OTA_UPGRADE_STATE_UPGRADING,
}OTA_UPGRADE_STATE;

/*ota 升级音乐播放*/
typedef enum
{
	OTA_PLAY_TONE_MSG_NONE,
	OTA_PLAY_TONE_MSG_START,
	OTA_PLAY_TONE_MSG_STOP,
}OTA_PLAY_TONE_MSG_T;

/*ota 任务运行状态*/
typedef enum
{
	OTA_TASK_STATUS_IDEL = 0,//空闲
	OTA_TASK_STATUS_RUNING,	 //运行
	OTA_TASK_STATUS_EXIT,	 //退出
}OTA_TASK_STATUS_T;

/*ota 升级方式*/
typedef enum
{
	MSG_OTA_IDEL,
	MSG_OTA_SD_UPDATE,
	MSG_OTA_WIFI_UPDATE
}MSG_OTA_SERVICE_T;

/*ota 运行句柄*/
typedef struct
{
	MediaService Based;
	OTA_UPGRADE_STATE ota_state;
	OTA_TASK_STATUS_T task_status_play_tone;
	OTA_TASK_STATUS_T task_status_ota_update;
	QueueHandle_t ota_queue;				
	QueueHandle_t ota_tone_queue;	
}OTA_SERVICE_T;

OTA_SERVICE_T *ota_service_create(void);
void ota_service_send_msg(MSG_OTA_SERVICE_T _msg);
OTA_UPGRADE_STATE get_ota_upgrade_state();

#endif
