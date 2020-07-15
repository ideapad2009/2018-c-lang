#ifndef _SD_MUSIC_MANAGE_H_
#define _SD_MUSIC_MANAGE_H_

#include <stdio.h>
#include "MediaService.h"
#include "auto_play_service.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//音乐资源类型
typedef enum
{
	FOLDER_INDEX_START = -1,
	FOLDER_INDEX_BAIKE = 0,
	FOLDER_INDEX_ERGE,
	FOLDER_INDEX_GUOXUE,
	FOLDER_INDEX_GUSHI,
	FOLDER_INDEX_KETANG,
	FOLDER_INDEX_QINZI,
	FOLDER_INDEX_XIAZAI,
	FOLDER_INDEX_ENGLISH,
	FOLDER_INDEX_MUSIC,
	FOLDER_INDEX_END
}FOLDER_INDEX_T;

typedef enum
{
	SD_MUSIC_MSG_MOUNT,
	SD_MUSIC_MSG_UNMOUNT,
	SD_MUSIC_MSG_QUIT,
}SD_MUSIC_MSG_T;

typedef enum
{
	SD_MUSIC_STATE_INIT,
	SD_MUSIC_STATE_PLAY,
	SD_MUSIC_STATE_STOP
}SD_MUSIC_STATE_T;
	
typedef struct
{
	xQueueHandle x_queue_sd_mount;
	AUTO_PLAY_MUSIC_SOURCE_T music_src;
	int is_need_switch_folder;
	FOLDER_INDEX_T last_folder_index;
}SD_MUSIC_MANAGE_HANDLE_T;

typedef struct
{
	char dir_path[32];
	char play_list_path[64];
	char play_list_tmp_path[64];
}TEMP_PATH_T;

void sd_music_manage_create(void);
void sd_music_manage_delete(void);
void sd_music_manage_mount(void);
void sd_music_manage_unmount(void);
void sd_music_start();
int get_sd_scan_status(void);
void set_sd_scan_status(int status);
void set_sd_switch_folder_status(int status);
int get_sd_switch_folder_status(void);
void set_sd_music_play_state();
void set_sd_music_stop_state();

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

