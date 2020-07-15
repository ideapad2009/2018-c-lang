
#ifndef _SD_FIRMWARE_UPDATE_H_
#define _SD_FIRMWARE_UPDATE_H_

#include "esp_ota_ops.h"
#include "esp_partition.h"

typedef enum 
{
	OTA_SUCCESS,
	OTA_INIT_FAIL,
	OTA_SD_FAIL,
	OTA_WIFI_FAIL,
	OTA_NO_SD_CARD,
	OTA_NEED_UPDATE_FW,
	OTA_NONEED_UPDATE_FW,
}OTA_ERROR_NUM_T;

OTA_ERROR_NUM_T ota_update_from_sd(const char *_firmware_path);
OTA_ERROR_NUM_T ota_update_from_wifi(const char *_firmware_url);
OTA_ERROR_NUM_T ota_update_check_fw_version(const char *_server_url, char *_out_fw_url, size_t _out_len);


#endif

