#ifndef __FLASH_CONFIG_MANAGE_H__
#define __FLASH_CONFIG_MANAGE_H__

#include "stdio.h"
#include "stdlib.h"
#include "esp_system.h"
#include "memo_service.h"

#define MAX_WIFI_NUM 			5 
#define DEVICE_PARAMS_VERSION_1 1
#define DEVICE_PARAMS_NAMESPACE "DEVICE_PARAMS"
#define DEVICE_PARAMS_NAME		"DEVICE_CONFIG"
#define DEFAULT_WIFI_SSID		CONFIG_ESP_AUDIO_WIFI_SSID
#define DEFAULT_WIFI_PASSWD		CONFIG_ESP_AUDIO_WIFI_PWD

//参数类型
typedef enum
{
	FLASH_CFG_START = 0,
	FLASH_CFG_FLASH_MUSIC_INDEX,
	FLASH_CFG_SDCARD_FOLDER_INDEX,
	FLASH_CFG_WIFI_INFO,
	FLASH_CFG_USER_ID,
	FLASH_CFG_DEVICE_ID,
	FLASH_CFG_MEMO_PARAMS,
	FLASH_CFG_OTA_MODE,//OTA升级模式，采用正式还是测试服务器
	FLASH_CFG_ASR_MODE,
	FLASH_CFG_APP_ID,
	FLASH_CFG_ROBOT_ID,
	FLASH_CFG_END
}FLASH_CONFIG_PARAMS_T;

//OTA升级模式
typedef enum
{
	OTA_UPDATE_MODE_FORMAL = 0x12345678, //正式版本
	OTA_UPDATE_MODE_TEST = 0x87654321,	//测试版本
}OTA_UPDATE_MODE_T;

typedef enum
{
	AI_ACOUNT_BAIDU = 0,
	AI_ACOUNT_SINOVOICE,
	AI_ACOUNT_ALL,
}AI_ACOUNT_T;

//nvs has only 16kb size

//total 64 bytes
typedef struct
{
	char wifi_ssid[32];		//wifi name
	char wifi_passwd[32];	//wifi password
}DEVICE_WIFI_INFO_T;

//total 324 bytes
typedef struct
{
	uint8_t wifi_storage_index;					// 最新wifi信息存储位置
	uint8_t wifi_connect_index;					// 最新连接wifi位置
	uint8_t res[2];
	DEVICE_WIFI_INFO_T wifi_info[MAX_WIFI_NUM];
}DEVICE_WIFI_INFOS_T;

//total 512 bytes
typedef struct
{
	char device_sn[32];		//device serial num
	char bind_user_id[32];	//user id,use to bind user
	char res[448];			//res
}DEVICE_BASIC_INFO_T;

//total size 1984 bytes,must 4 bytes align
typedef struct
{
	// 1.param version 4 bytes
	uint32_t params_version;
	
	// 2.wifi info 324 bytes
	DEVICE_WIFI_INFOS_T wifi_infos;
	
	// 3.device basic info 512 bytes
	DEVICE_BASIC_INFO_T basic_info;

	// 4.other params
	uint8_t flash_music_index;		// falsh music play index
	uint8_t sd_music_folder_index;	// sd music folder index
	uint8_t res1[2];
	MEMO_ARRAY_T memo_array;

	//5.Remaining
	uint32_t ota_mode;//ota update mode
	uint8_t res2[452];
	
}DEVICE_PARAMS_CONFIG_T;

typedef struct
{
	char asr_url[64];
	char tts_url[64];
	char app_key[64];
	char dev_key[64];
}SINOVOICE_ACOUNT_T;

typedef struct
{
	char asr_url[64];
	char tts_url[64];
	char app_id[16];
	char app_key[64];
	char secret_key[64];
}BAIDU_ACOUNT_T;

typedef struct
{
	SINOVOICE_ACOUNT_T st_sinovoice_acount;
	BAIDU_ACOUNT_T st_baidu_acount;
}PLATFORM_AI_ACOUNTS_T;

esp_err_t get_flash_cfg(FLASH_CONFIG_PARAMS_T _params, void *_value);
esp_err_t set_flash_cfg(FLASH_CONFIG_PARAMS_T _params, void *_value);
int init_device_params(void);
void print_wifi_infos(DEVICE_WIFI_INFOS_T* _wifi_infos);
void print_device_params(void);
void get_ai_acount(AI_ACOUNT_T _type, void *_out);
void set_ai_acount(AI_ACOUNT_T _type, void *_out);
#endif

