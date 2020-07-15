
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
#include <dirent.h>

#include "cJSON.h"
//#include "DeepBrainService.h"
#include "touchpad.h"
#include "userconfig.h"
#include "DeviceCommon.h"
#include "MediaControl.h"
#include "MediaHal.h"
#include "toneuri.h"
#include "baidu_api.h"
#include "http_api.h"
#include "deepbrain_api.h"
#include "device_api.h"
//#include "sinovoice_api.h"
#include "socket.h"
#include "lwip/netdb.h"
#include "EspAudio.h"
#include "sd_music_manage.h"
#include "flash_config_manage.h"
#include "nvs.h"
#include "SDCardConfig.h"
#include "TouchControlService.h"
//#include "collection_function.h"
#include "player_middleware.h"
#include "auto_play_service.h"
#include "flash_music_manage.h"
#include "free_talk.h"

#define PRINT_TAG	"sd music manage"
#define SD_PLAYER_CTRL_SER_TAG	PRINT_TAG
#define SD_PLAY_LIST_NAME "play_list.bin"
#define SD_PLAY_LIST_TMP_NAME "play_list_tmp.bin"

static int g_sd_scan_status = 0;
static SD_MUSIC_STATE_T g_sd_music_state = SD_MUSIC_STATE_INIT;

typedef struct
{
	unsigned char str_name[16];			
}FOLDER_NAME_T;

//playlist header 128字节
typedef struct
{
	unsigned int total_num;
	unsigned int current_num;
	unsigned char res[120];
}SD_PLAY_LIST_HEADER_T;

//音乐节点
typedef struct
{
	unsigned char str_music_path[128];			
}SD_MUSIC_NODE_T;

typedef enum
{
	SD_MUSIC_CTRL_PREV,//上一曲
	SD_MUSIC_CTRL_NEXT,//下一曲
	SD_MUSIC_CTRL_CURR,//当前一曲
}SD_MUSIC_CTRL_T;

typedef enum
{
	SD_MUSIC_ERRNO_SUCCESS,
	SD_MUSIC_ERRNO_NO_MUSIC,
	SD_MUSIC_ERRNO_READ_ERROR,
	SD_MUSIC_ERRNO_WRITE_ERROR,
	SD_MUSIC_ERRNO_REACH_FIRST,
	SD_MUSIC_ERRNO_REACH_LAST,
	SD_MUSIC_ERRNO_MUSIC_URL_ERROR,
	SD_MUSIC_ERRNO_NO_SD,
	SD_MUSIC_ERRNO_SD_SCAN,
}SD_MUSIC_ERRNO_T;

typedef struct
{
	char play_list_path[64];
	SD_MUSIC_NODE_T music_node;
	SD_PLAY_LIST_HEADER_T play_list_header;
}TEMP_LIST_T;

typedef struct
{
	SD_MUSIC_NODE_T	music_node_new;
	SD_MUSIC_NODE_T	music_node_old;
	SD_PLAY_LIST_HEADER_T play_list_header_new;
	SD_PLAY_LIST_HEADER_T play_list_header_old;
}TEMP_COMPARE_T;

static const FOLDER_NAME_T g_folder_list[] = 
{
	{{0xB0,0xD9,0xBF,0xC6}},//百科
	{{0xB6,0xF9,0xB8,0xE8}},//儿歌
	{{0xB9,0xFA,0xD1,0xA7}},//国学
	{{0xB9,0xCA,0xCA,0xC2}},//故事
	{{0xBF,0xCE,0xCC,0xC3}},//课堂
	{{0xC7,0xD7,0xD7,0xD3}},//亲子
	{{0xCF,0xC2,0xD4,0xD8}},//下载
	{{0xD3,0xA2,0xD3,0xEF}},//英语
	{{0xD2,0xF4,0xC0,0xD6}},//音乐
};

#define SD_FOLDER_LIST_SIZE (sizeof(g_folder_list)/sizeof(FOLDER_NAME_T))

SD_MUSIC_MANAGE_HANDLE_T *g_sd_music_handle = NULL;

static unsigned char * get_sd_folder_name(int _index)
{
	return &g_folder_list[_index];
}

static SD_MUSIC_ERRNO_T get_sd_music_url(
	AUTO_PLAY_MUSIC_SOURCE_T *src, FOLDER_INDEX_T folder_index, int switch_flag, SD_MUSIC_CTRL_T music_ctrl)
{
	FILE *play_list_fp = NULL;
	int ret = 0;
	int err_no = SD_MUSIC_ERRNO_SUCCESS;

	TEMP_LIST_T *temp = (TEMP_LIST_T *)esp32_malloc(sizeof(TEMP_LIST_T));
	if (NULL == temp)
	{
		err_no = SD_MUSIC_ERRNO_NO_MUSIC;
		goto _play_sd_music_error;
	}
	memset(temp, 0, sizeof(TEMP_LIST_T));
	
	//open play list
	snprintf(temp->play_list_path, sizeof(temp->play_list_path), "/sdcard/%s/%s", 
		get_sd_folder_name(folder_index), SD_PLAY_LIST_NAME);
	play_list_fp = fopen(temp->play_list_path, "rb+");
	if (play_list_fp == NULL)
	{
		err_no = SD_MUSIC_ERRNO_NO_MUSIC;
		ESP_LOGE(PRINT_TAG, "play_list_path=%s failed", temp->play_list_path);
		goto _play_sd_music_error;
	}
	
	//read play list head
	fseek(play_list_fp, 0, SEEK_SET);
	memset(&temp->play_list_header, 0, sizeof(temp->play_list_header));
	ret = fread(&temp->play_list_header, sizeof(temp->play_list_header), 1, play_list_fp);
	if (ret <= 0)
	{
		err_no = SD_MUSIC_ERRNO_NO_MUSIC;
		ESP_LOGE(PRINT_TAG, "play_list_path=%s no music", temp->play_list_path);
		goto _play_sd_music_error;
	}

	if (switch_flag)
	{
		temp->play_list_header.current_num = temp->play_list_header.total_num;
	}
	
	switch (music_ctrl)
	{
		case SD_MUSIC_CTRL_CURR:
		{
			break;
		}
		case SD_MUSIC_CTRL_PREV:
		{
			if (temp->play_list_header.current_num == 0)
			{
				err_no = SD_MUSIC_ERRNO_REACH_FIRST;
			}
			else
			{
				temp->play_list_header.current_num--;
			}
			break;
		}
		case SD_MUSIC_CTRL_NEXT:
		{
			if (temp->play_list_header.current_num == (temp->play_list_header.total_num - 1))
			{
				temp->play_list_header.current_num = 0;
				err_no = SD_MUSIC_ERRNO_REACH_LAST;
			}
			else
			{
				temp->play_list_header.current_num++;
			}
			break;
		}
		default:
			break;
	}

	ESP_LOGI(PRINT_TAG, "current_num=%d,total_num=%d", 
		temp->play_list_header.current_num, temp->play_list_header.total_num);

	if (temp->play_list_header.total_num == 0)
	{
		err_no = SD_MUSIC_ERRNO_NO_MUSIC;
		goto _play_sd_music_error;
	}
	
	fseek(play_list_fp, sizeof(temp->play_list_header)+sizeof(temp->music_node)*temp->play_list_header.current_num, SEEK_SET);
	memset(&temp->music_node , 0, sizeof(temp->music_node));
	ret = fread(&temp->music_node, sizeof(temp->music_node), 1, play_list_fp);
	if (ret <= 0)
	{
		err_no = SD_MUSIC_ERRNO_READ_ERROR;
		ESP_LOGE(PRINT_TAG, "play_list_path=%s, fread music_node failed", temp->play_list_path);
		goto _play_sd_music_error;
	} 

	if (strlen((char*)temp->music_node.str_music_path) > 0 
		&& strlen((char*)temp->music_node.str_music_path) <= (sizeof(temp->music_node.str_music_path)-1))
	{
		snprintf(src->music_url, sizeof(src->music_url), "%s", temp->music_node.str_music_path);
		ESP_LOGI(PRINT_TAG, "str_music_path=%s", src->music_url);
	}
	else
	{
		err_no = SD_MUSIC_ERRNO_MUSIC_URL_ERROR;
		goto _play_sd_music_error;
	}

	fseek(play_list_fp, 0, SEEK_SET);
	if (fwrite(&temp->play_list_header, sizeof(temp->play_list_header), 1, play_list_fp) <= 0)
	{
		ESP_LOGE(PRINT_TAG, "fwrite play_list_header failed");
		err_no = SD_MUSIC_ERRNO_WRITE_ERROR;
		goto _play_sd_music_error;
	}

_play_sd_music_error:
	if (play_list_fp != NULL)
	{
		fclose(play_list_fp);
	}

	if (NULL != temp)
	{
		esp32_free(temp);
		temp = NULL;
	}

	return err_no;
}

static SD_MUSIC_ERRNO_T detect_sd_card_status(void)
{
	if (sd_card_insert_status() == 0)
	{
		player_mdware_play_tone(FLASH_MUSIC_NO_TF_CARD);
		return SD_MUSIC_ERRNO_NO_SD;
	}

	if (get_sd_scan_status() == 1)
	{
		player_mdware_play_tone(FLASH_MUSIC_WAIT_FOR_SD_SCAN);
		return SD_MUSIC_ERRNO_SD_SCAN;
	}

	return SD_MUSIC_ERRNO_SUCCESS;
}

static int is_valid_music_name(const char *_music_name)
{
	char *str_name = (char *)esp32_malloc(128);
	if (NULL == str_name)
	{
		return ESP_FAIL;
	}
	memset(str_name, 0, 128);

	snprintf(str_name, 128, "%s", _music_name);
	strlwr(str_name);
	if (strstr(str_name, ".mp4") != NULL
		|| strstr(str_name, ".m4a") != NULL
		|| strstr(str_name, ".aac") != NULL
		|| strstr(str_name, ".ogg") != NULL
		|| strstr(str_name, ".flac") != NULL
		|| strstr(str_name, ".opus") != NULL
		|| strstr(str_name, ".amr") != NULL
		|| strstr(str_name, ".mp3") != NULL
		|| strstr(str_name, ".wav") != NULL
		|| strstr(str_name, ".pcm") != NULL)
	{
		if (NULL != str_name)
		{
			esp32_free(str_name);
			str_name = NULL;
		}
		return ESP_OK;
	}
		
	if (NULL != str_name)
	{
		esp32_free(str_name);
		str_name = NULL;
	}
	return ESP_FAIL;
}

static int create_new_play_list(const char* _p_dir_path, DIR *_p_dir, FILE *_p_play_list)
{
	int file_count = 0;
	struct dirent *p_dir_node = NULL;

	TEMP_LIST_T *temp = (TEMP_LIST_T *)esp32_malloc(sizeof(TEMP_LIST_T));
	if (NULL == temp)
	{
		return ESP_FAIL;
	}
	memset(temp, 0, sizeof(TEMP_LIST_T));
	
	
	memset(&temp->play_list_header, 0, sizeof(temp->play_list_header));
	if (fwrite(&temp->play_list_header, sizeof(temp->play_list_header), 1, _p_play_list) <= 0)
	{
		DEBUG_LOGE(SD_PLAYER_CTRL_SER_TAG, "fwrite play_list_header failed");
		esp32_free(temp);
		return ESP_FAIL;
	}

	file_count = 0;
	do
	{
		p_dir_node = readdir(_p_dir);
		if (p_dir_node == NULL)
		{
			break;
		}

		DEBUG_LOGE(SD_PLAYER_CTRL_SER_TAG, "%s", p_dir_node->d_name);
		if (strlen(p_dir_node->d_name) > 0 && is_valid_music_name(p_dir_node->d_name) == ESP_OK)
		{
			//读取list内容，进行比较
			file_count++;
			memset(&temp->music_node, 0, sizeof(temp->music_node));
			snprintf((char*)temp->music_node.str_music_path, sizeof(temp->music_node.str_music_path), "file://%s/%s", _p_dir_path, p_dir_node->d_name);
			if (fwrite(&temp->music_node, sizeof(temp->music_node), 1, _p_play_list) <= 0)
			{
				DEBUG_LOGE(SD_PLAYER_CTRL_SER_TAG, "fwrite:%s failed", p_dir_node->d_name);
				if (NULL != temp)
				{
					esp32_free(temp);
					temp = NULL;
				}
				return ESP_FAIL;
			}
		}
	}
	while (p_dir_node != NULL);
	
	if (file_count > 0)
	{
		temp->play_list_header.total_num = file_count;
		temp->play_list_header.current_num = 0;
		fseek(_p_play_list, 0, SEEK_SET);
		if (fwrite(&temp->play_list_header, sizeof(temp->play_list_header), 1, _p_play_list) <= 0)
		{
			DEBUG_LOGE(SD_PLAYER_CTRL_SER_TAG, "fwrite play list header failed");
			if (NULL != temp)
			{
				esp32_free(temp);
				temp = NULL;
			}
			return ESP_FAIL;
		}
	}

	if (NULL != temp)
	{
		esp32_free(temp);
		temp = NULL;
	}

	return ESP_OK;
}


static int compare_play_list(FILE *_p_play_list_new, FILE *_p_play_list_old)
{
	int ret = 0;
	int file_count = 0;

	TEMP_COMPARE_T *temp = (TEMP_COMPARE_T *)esp32_malloc(sizeof(TEMP_COMPARE_T));
	if (NULL == temp)
	{
		goto compare_play_list_err;
	}
	memset(temp, 0, sizeof(TEMP_COMPARE_T));

	fseek(_p_play_list_new, 0, SEEK_SET);
	fseek(_p_play_list_old, 0, SEEK_SET);
	memset(&temp->play_list_header_new, 0, sizeof(temp->play_list_header_new));
	ret = fread(&temp->play_list_header_new, sizeof(temp->play_list_header_new), 1, _p_play_list_new);
	if (ret <= 0)
	{
		goto compare_play_list_err;
	}
	
	memset(&temp->play_list_header_old, 0, sizeof(temp->play_list_header_old));
	ret = fread(&temp->play_list_header_old, sizeof(temp->play_list_header_old), 1, _p_play_list_old);
	if (ret <= 0)
	{
		goto compare_play_list_err;
	}

	if (temp->play_list_header_new.total_num != temp->play_list_header_old.total_num)
	{		
		 if (NULL != temp)
		{
			esp32_free(temp);
			temp = NULL;
		}
		return 1;
	}

	do
	{
		memset(&temp->music_node_new, 0, sizeof(temp->music_node_new));
		ret = fread(&temp->music_node_new, sizeof(temp->music_node_new), 1, _p_play_list_new);
		if (ret <= 0)
		{
			break;
		} 

		memset(&temp->music_node_old, 0, sizeof(temp->music_node_old));
		ret = fread(&temp->music_node_old, sizeof(temp->music_node_old), 1, _p_play_list_old);
		if (ret <= 0)
		{
			break;
		} 

		if (strcmp((char*)temp->music_node_new.str_music_path, (char*)temp->music_node_old.str_music_path)!= 0)
		{
			goto compare_play_list_err;
		}
		
		file_count++;
	}while(1);

	if (file_count != temp->play_list_header_new.total_num)
	{
		goto compare_play_list_err;
	}

	 if (NULL != temp)
	{
		esp32_free(temp);
		temp = NULL;
	}

	return 0;
compare_play_list_err:
	if (NULL != temp)
	{
		esp32_free(temp);
		temp = NULL;
	}
	return -1;
}


static int scan_music_dir(const unsigned char * _dir_name)
{
	int ret = ESP_FAIL;
	DIR *p_dir = NULL;
	struct dirent *p_dir_node = NULL;
	FILE *play_list_fp = NULL;
	FILE *play_list_tmp_fp = NULL;
	int b_play_list_exit = 0;

	TEMP_PATH_T *temp = (TEMP_PATH_T *)esp32_malloc(sizeof(TEMP_PATH_T));
	if (NULL == temp)
	{
		return ESP_FAIL;
	}
	memset(temp, 0, sizeof(TEMP_PATH_T));
		
#if 0
	int i = 0;
	for (i=0; i<strlen((char*)_dir_name); i++)
	{
		printf("%02X ", (unsigned char)_dir_name[i]);
	}
	printf("\n");
#endif
	snprintf(temp->dir_path, sizeof(temp->dir_path), "/sdcard/%s", _dir_name);
	p_dir = opendir(temp->dir_path);
	if (p_dir == NULL)
	{
		//失败后，创建目录
		mkdir(temp->dir_path, S_IRWXU|S_IRWXG|S_IRWXO);
		DEBUG_LOGE(SD_PLAYER_CTRL_SER_TAG, "open %s failed", temp->dir_path);
		esp32_free(temp);
		return ESP_OK;
	}

	//打开play list 和 tmp play list
	snprintf(temp->play_list_path, sizeof(temp->play_list_path), "%s/%s", temp->dir_path, SD_PLAY_LIST_NAME);
	snprintf(temp->play_list_tmp_path, sizeof(temp->play_list_tmp_path), "%s/%s", temp->dir_path, SD_PLAY_LIST_TMP_NAME);
	play_list_fp = fopen(temp->play_list_path, "rb");
	if (play_list_fp != NULL)
	{
		b_play_list_exit = 1;
	}

	play_list_tmp_fp = fopen(temp->play_list_tmp_path, "w+");
	if (play_list_tmp_fp == NULL)
	{
		goto _scan_music_dir_error;
	}

	//创建新的播放列表
	if (create_new_play_list(temp->dir_path, p_dir, play_list_tmp_fp) != ESP_OK)
	{
		goto _scan_music_dir_error;
	}

	//比较新列表和旧列表是否相同
	if (b_play_list_exit)
	{
		if (compare_play_list(play_list_tmp_fp, play_list_fp) != 0)
		{
			if (play_list_fp != NULL)
			{
				fclose(play_list_fp);
				play_list_fp = NULL;
			}

			if (play_list_tmp_fp != NULL)
			{
				fclose(play_list_tmp_fp);
				play_list_tmp_fp = NULL;
			}
	
			if (remove(temp->play_list_path) != 0)
			{
				DEBUG_LOGE(SD_PLAYER_CTRL_SER_TAG, "remove %s fail", temp->play_list_path);
			}
			
		 	if (rename(temp->play_list_tmp_path, temp->play_list_path) != 0)
		 	{
				DEBUG_LOGE(SD_PLAYER_CTRL_SER_TAG, "rename %s to %s fail", temp->play_list_tmp_path, temp->play_list_path);
			}
		}
		else
		{
			DEBUG_LOGE(SD_PLAYER_CTRL_SER_TAG, "%s play list is same", temp->dir_path);
			if (play_list_tmp_fp != NULL)
			{
				fclose(play_list_tmp_fp);
				play_list_tmp_fp = NULL;
			}
			
			if (remove(temp->play_list_tmp_path) != 0)
			{
				DEBUG_LOGE(SD_PLAYER_CTRL_SER_TAG, "remove %s fail", temp->play_list_tmp_path);
			}
		}
	}
	else
	{
		if (play_list_fp != NULL)
		{
			fclose(play_list_fp);
			play_list_fp = NULL;
		}

		if (play_list_tmp_fp != NULL)
		{
			fclose(play_list_tmp_fp);
			play_list_tmp_fp = NULL;
		}
	
	 	if (rename(temp->play_list_tmp_path, temp->play_list_path) != 0)
	 	{
			DEBUG_LOGE(SD_PLAYER_CTRL_SER_TAG, "rename %s to %s fail", temp->play_list_tmp_path, temp->play_list_path);
		}
	}

	ret = ESP_OK;
_scan_music_dir_error:
	if (play_list_fp != NULL)
	{
		fclose(play_list_fp);
		play_list_fp = NULL;
	}

	if (play_list_tmp_fp != NULL)
	{
		fclose(play_list_tmp_fp);
		play_list_tmp_fp = NULL;
	}

	if (p_dir != NULL)
	{
		closedir(p_dir);
		p_dir = NULL;
	}

	if (NULL != temp)
	{
		esp32_free(temp);
		temp = NULL;
	}

	return ret;
}

/*
	当sd卡插入后，重新初始化
	
*/
static void task_scan_sd_music(void *pv)
{
	int i = 0;
	int msg = 0;
	int first_time = 1;
	SD_MUSIC_MANAGE_HANDLE_T *sd_music_manage = (SD_MUSIC_MANAGE_HANDLE_T *)pv;
	
    while (1) 
	{		
		if (xQueueReceive(sd_music_manage->x_queue_sd_mount, &msg, (TickType_t)10*1000) == pdPASS) 
		{
			switch (msg)
			{
				case SD_MUSIC_MSG_MOUNT:
				{
					if (is_free_talk_running())
					{
						free_talk_stop();//关闭录音
						auto_play_stop();
					}
					player_mdware_play_tone(FLASH_MUSIC_TF_CARD_INSERTED);
					vTaskDelay(2000);
					set_sd_scan_status(1);
					//遍历所有目录
					for (i=0; i < SD_FOLDER_LIST_SIZE; i++)
					{
						if (scan_music_dir(get_sd_folder_name(i)) != ESP_OK)
						{
							break;
						}
					}
					set_sd_scan_status(0);
					sd_music_start();
					break;
				}
				case SD_MUSIC_MSG_UNMOUNT:
				{
					if (is_free_talk_running())
					{
						free_talk_stop();//关闭录音
						auto_play_stop();
					}
					player_mdware_play_tone(FLASH_MUSIC_TF_CARD_PULL_OUT);
					break;
				}
				case SD_MUSIC_MSG_STOP_FIRST_PLAY:
				{
					first_time = 0;
					break;
				}
				default:
					break;				
			}

			if (msg == SD_MUSIC_MSG_QUIT)
			{
				break;
			}
		}
		else
		{
			if (first_time == 1)
			{
				if (sd_card_insert_status() == 1)
				{
					sd_music_start();
				}
				else
				{
					//flash_music_start();
				}
				first_time = 0;
			}
		}
    }
    vTaskDelete(NULL);
}

static void get_prev_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	int ret = 0;
	int switch_flag = 0;
	AUTO_PLAY_MUSIC_SOURCE_T music_src = {0};
	FOLDER_INDEX_T folder_index = FOLDER_INDEX_START;
	
	if (g_sd_music_handle == NULL)
	{
		return;
	}
	memset(&music_src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));

	if (detect_sd_card_status() != SD_MUSIC_ERRNO_SUCCESS)
	{
		return;
	}

	ret = get_flash_cfg(FLASH_CFG_SDCARD_FOLDER_INDEX, &folder_index);
	if (ret != ESP_OK)
	{
		ESP_LOGE(PRINT_TAG, "get_flash_cfg FLASH_CFG_SDCARD_FOLDER_INDEX failed");
	}

	if (folder_index <= FOLDER_INDEX_START || folder_index >= FOLDER_INDEX_END)
	{
		folder_index = FOLDER_INDEX_BAIKE;
	}

	//ESP_LOGE(PRINT_TAG, "prev********folder_index=%d", folder_index);

	ret = set_flash_cfg(FLASH_CFG_SDCARD_FOLDER_INDEX, &folder_index);
	if (ret != ESP_OK)
	{
		ESP_LOGE(PRINT_TAG, "set_flash_cfg 1 FLASH_CFG_SDCARD_FOLDER_INDEX failed");
	}

	int first_folder_index = folder_index;
	do 
	{
		ret = get_sd_music_url(&music_src, folder_index, switch_flag, SD_MUSIC_CTRL_PREV);
		if (ret == SD_MUSIC_ERRNO_SUCCESS)
		{
			g_sd_music_handle->music_src = music_src;
			*src = music_src;
			break;
		}
		else if (ret == SD_MUSIC_ERRNO_NO_MUSIC)
		{
			
		}
		else if (ret == SD_MUSIC_ERRNO_READ_ERROR)
		{
			
		}
		else if (ret == SD_MUSIC_ERRNO_WRITE_ERROR)
		{
			
		}
		else if (ret == SD_MUSIC_ERRNO_REACH_FIRST)
		{
			switch_flag = 1;
		}
		else if (ret == SD_MUSIC_ERRNO_REACH_LAST)
		{
			
		}
		else if(ret == SD_MUSIC_ERRNO_MUSIC_URL_ERROR)
		{
			
		}
		else
		{
			
		}

		folder_index--;
		if (folder_index <= FOLDER_INDEX_START)
		{
			folder_index = FOLDER_INDEX_MUSIC;
		}

		if (folder_index == first_folder_index)
		{	
			break;
		}
		
		ret = set_flash_cfg(FLASH_CFG_SDCARD_FOLDER_INDEX, &folder_index);
		if (ret != ESP_OK)
		{
			ESP_LOGE(PRINT_TAG, "set_flash_cfg 2 FLASH_CFG_SDCARD_FOLDER_INDEX failed");
		}			
	}while(1);
}



static void get_next_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	int ret = 0;
	AUTO_PLAY_MUSIC_SOURCE_T music_src = {0};
	FOLDER_INDEX_T folder_index = FOLDER_INDEX_START;
	
	if (g_sd_music_handle == NULL)
	{
		return;
	}
	memset(&music_src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));

	if (detect_sd_card_status() != SD_MUSIC_ERRNO_SUCCESS)
	{
		return;
	}

	ret = get_flash_cfg(FLASH_CFG_SDCARD_FOLDER_INDEX, &folder_index);
	if (ret != ESP_OK)
	{
		ESP_LOGE(PRINT_TAG, "get_flash_cfg FLASH_CFG_SDCARD_FOLDER_INDEX failed");
	}

	if (folder_index <= FOLDER_INDEX_START || folder_index >= FOLDER_INDEX_END)
	{
		folder_index = FOLDER_INDEX_BAIKE;
	}

	//ESP_LOGE(PRINT_TAG, "next********folder_index=%d", folder_index);

	ret = set_flash_cfg(FLASH_CFG_SDCARD_FOLDER_INDEX, &folder_index);
	if (ret != ESP_OK)
	{
		ESP_LOGE(PRINT_TAG, "set_flash_cfg 1 FLASH_CFG_SDCARD_FOLDER_INDEX failed");
	}

	int first_folder_index = folder_index;
	do 
	{
		ret = get_sd_music_url(&music_src, folder_index, 0, SD_MUSIC_CTRL_NEXT);
		if (ret == SD_MUSIC_ERRNO_SUCCESS)
		{
			g_sd_music_handle->music_src = music_src;
			*src = music_src;
			break;
		}
		else if (ret == SD_MUSIC_ERRNO_NO_MUSIC)
		{
			
		}
		else if (ret == SD_MUSIC_ERRNO_READ_ERROR)
		{
			
		}
		else if (ret == SD_MUSIC_ERRNO_WRITE_ERROR)
		{
			
		}
		else if (ret == SD_MUSIC_ERRNO_REACH_FIRST)
		{
			
		}
		else if (ret == SD_MUSIC_ERRNO_REACH_LAST)
		{
			
		}
		else if(ret == SD_MUSIC_ERRNO_MUSIC_URL_ERROR)
		{
			
		}
		else
		{
			
		}

		folder_index++;
		if (folder_index >= FOLDER_INDEX_END)
		{
			folder_index = FOLDER_INDEX_BAIKE;
		}

		if (folder_index == first_folder_index)
		{
			break;
		}
		
		ret = set_flash_cfg(FLASH_CFG_SDCARD_FOLDER_INDEX, &folder_index);
		if (ret != ESP_OK)
		{
			ESP_LOGE(PRINT_TAG, "set_flash_cfg 2 FLASH_CFG_SDCARD_FOLDER_INDEX failed");
		}
		
	}while(1);
}

static void get_current_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	int ret = 0;
	FOLDER_INDEX_T folder_index = FOLDER_INDEX_START;
	
	if (src == NULL || g_sd_music_handle == NULL)
	{
		return;
	}
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	
	if (detect_sd_card_status() != SD_MUSIC_ERRNO_SUCCESS)
	{
		return;
	}

	*src = g_sd_music_handle->music_src;
}

void sd_music_manage_create(void)
{
    g_sd_music_handle = (SD_MUSIC_MANAGE_HANDLE_T *)esp32_malloc(sizeof(SD_MUSIC_MANAGE_HANDLE_T));
	if (g_sd_music_handle == NULL)
	{
		ESP_LOGE(PRINT_TAG, "sd_music_manage_create failed");
		return;
	}
	memset(g_sd_music_handle, 0, sizeof(SD_MUSIC_MANAGE_HANDLE_T));
	
	g_sd_music_handle->x_queue_sd_mount = xQueueCreate(1, sizeof(int));
	if (g_sd_music_handle->x_queue_sd_mount == NULL)
	{
		ESP_LOGE(PRINT_TAG, "x_queue_sd_mount create failed");
		esp32_free(g_sd_music_handle);
		g_sd_music_handle = NULL;
		return;
	}

	if (xTaskCreate(task_scan_sd_music,
				    "task_scan_sd_music",
				    1024*3,
				    g_sd_music_handle,
				    4,
				    NULL) != pdPASS) 
    {

        ESP_LOGE(PRINT_TAG, "ERROR creating task_scan_sd_music task! Out of memory?");
    }
    
    ESP_LOGE(PRINT_TAG, "sd_music_manage_create\r\n");
}

void sd_music_manage_mount(void)
{
	int msg = SD_MUSIC_MSG_MOUNT;
	
	if (g_sd_music_handle != NULL && g_sd_music_handle->x_queue_sd_mount != NULL)
	{
		xQueueSend(g_sd_music_handle->x_queue_sd_mount, &msg, 0);
	}
}

void sd_music_manage_unmount(void)
{
	int msg = SD_MUSIC_MSG_UNMOUNT;
	
	if (g_sd_music_handle != NULL && g_sd_music_handle->x_queue_sd_mount != NULL)
	{
		xQueueSend(g_sd_music_handle->x_queue_sd_mount, &msg, 0);
	}
}

void sd_music_manage_stop_first_play(void)
{
	int msg = SD_MUSIC_MSG_STOP_FIRST_PLAY;
	
	if (g_sd_music_handle != NULL && g_sd_music_handle->x_queue_sd_mount != NULL)
	{
		xQueueSend(g_sd_music_handle->x_queue_sd_mount, &msg, 0);
	}
}


void sd_music_manage_delete(void)
{
	int msg = SD_MUSIC_MSG_QUIT;
	
	if (g_sd_music_handle != NULL && g_sd_music_handle->x_queue_sd_mount != NULL)
	{
		xQueueSend(g_sd_music_handle->x_queue_sd_mount, &msg, 0);
	}
}

int get_sd_switch_folder_status(void)
{
	if (g_sd_music_handle != NULL)
	{
		return g_sd_music_handle->is_need_switch_folder;
	}

	return 0;
}

void set_sd_switch_folder_status(int status)
{
	if (g_sd_music_handle != NULL)
	{
		g_sd_music_handle->is_need_switch_folder = status;
	}
}

void set_sd_music_play_state()
{
	g_sd_music_state = SD_MUSIC_STATE_PLAY;
}

void set_sd_music_stop_state()
{
	g_sd_music_state = SD_MUSIC_STATE_STOP;
}

int get_sd_scan_status(void)
{
	return g_sd_scan_status;
}

void set_sd_scan_status(int status)
{
	g_sd_scan_status = status;
}

void sd_music_start()
{
	int ret = 0;
	AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_music, get_next_music, get_current_music};
	FOLDER_INDEX_T folder_index = FOLDER_INDEX_START;
	static FOLDER_INDEX_T last_folder_index = FOLDER_INDEX_START;

	AUTO_PLAY_MUSIC_SOURCE_T *music_src = (AUTO_PLAY_MUSIC_SOURCE_T *)esp32_malloc(sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (g_sd_music_handle == NULL || music_src == NULL)
	{
		return;
	}
	memset(music_src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));

	if (detect_sd_card_status() != SD_MUSIC_ERRNO_SUCCESS)
	{
		if (music_src != NULL)
		{
			esp32_free(music_src);
			music_src = NULL;
		}
		
		return;
	}

	ret = get_flash_cfg(FLASH_CFG_SDCARD_FOLDER_INDEX, &folder_index);
	if (ret != ESP_OK)
	{
		ESP_LOGE(PRINT_TAG, "get_flash_cfg FLASH_CFG_SDCARD_FOLDER_INDEX failed");
	}

	if (folder_index <= FOLDER_INDEX_START || folder_index >= FOLDER_INDEX_END)
	{
		folder_index = FOLDER_INDEX_BAIKE;
	}
	else
	{
		AUTO_PLAY_CALL_BACK_T cur_callbacks = {0};
		get_auto_play_cb(&cur_callbacks);
		
		folder_index++;
		if (folder_index >= FOLDER_INDEX_END)
		{
			 folder_index = FOLDER_INDEX_BAIKE;
		}

		if (cur_callbacks.now == call_backs.now)
		{
		 	
		}
	}	
	last_folder_index = folder_index;

	ret = set_flash_cfg(FLASH_CFG_SDCARD_FOLDER_INDEX, &folder_index);
	if (ret != ESP_OK)
	{
		ESP_LOGE(PRINT_TAG, "set_flash_cfg FLASH_CFG_SDCARD_FOLDER_INDEX failed");
	}

	switch (folder_index)
	{
		case FOLDER_INDEX_BAIKE:
		{
			player_mdware_play_tone(FLASH_MUSIC_BAI_KE);
			break;
		}
		case FOLDER_INDEX_ERGE:
		{
			player_mdware_play_tone(FLASH_MUSIC_ER_GE);
			break;
		}
		case FOLDER_INDEX_GUOXUE:
		{
			player_mdware_play_tone(FLASH_MUSIC_GUO_XUE);
			break;
		}
		case FOLDER_INDEX_GUSHI:
		{
			player_mdware_play_tone(FLASH_MUSIC_GU_SHI);
			break;
		}
		case FOLDER_INDEX_KETANG:
		{
			player_mdware_play_tone(FLASH_MUSIC_KE_TANG);
			break;
		}
		case FOLDER_INDEX_QINZI:
		{
			player_mdware_play_tone(FLASH_MUSIC_QIN_ZI);
			break;
		}
		case FOLDER_INDEX_XIAZAI:
		{
			player_mdware_play_tone(FLASH_MUSIC_XIA_ZAI);
			break;
		}
		case FOLDER_INDEX_ENGLISH:
		{
			player_mdware_play_tone(FLASH_MUSIC_YING_YU);
			break;
		}
		case FOLDER_INDEX_MUSIC:
		{
			player_mdware_play_tone(FLASH_MUSIC_YIN_YUE);
			break;
		}
		default:
			break;
	}
	vTaskDelay(2000);

	ret = get_sd_music_url(music_src, folder_index, 0, SD_MUSIC_CTRL_CURR);
	switch (ret)
	{
		case SD_MUSIC_ERRNO_SUCCESS:
		{
			g_sd_music_handle->music_src = *music_src;
			set_sd_music_play_state();
			set_auto_play_cb(&call_backs);
			set_sd_switch_folder_status(1);
			break;
		}
		case SD_MUSIC_ERRNO_NO_MUSIC:
		{
			player_mdware_play_tone(FLASH_MUSIC_NO_MUSIC);
			printf("FLASH_MUSIC_NO_MUSIC 3!\n");
			set_sd_switch_folder_status(1);
			break;
		}
		case SD_MUSIC_ERRNO_READ_ERROR:
		{
			set_sd_switch_folder_status(1);
			break;
		}
		case SD_MUSIC_ERRNO_WRITE_ERROR:
		{
			set_sd_switch_folder_status(1);
			break;
		}
		case SD_MUSIC_ERRNO_REACH_FIRST:
		{
			set_sd_switch_folder_status(1);
			break;
		}
		case SD_MUSIC_ERRNO_REACH_LAST:
		{
			set_sd_switch_folder_status(1);
			break;
		}
		default:
			break;
	}	
	if (music_src != NULL)
	{
		esp32_free(music_src);
		music_src = NULL;
	}
}

