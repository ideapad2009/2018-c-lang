#ifndef _SD_MUSIC_MANAGE_H_
#define _SD_MUSIC_MANAGE_H_

#include <stdio.h>
#include "MediaService.h"
#include "auto_play_service.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

typedef enum
{
	SD_MUSIC_MSG_MOUNT,
	SD_MUSIC_MSG_UNMOUNT,
	SD_MUSIC_MSG_QUIT,
}SD_MUSIC_MSG_T;

typedef struct
{
	xQueueHandle x_queue_sd_mount;
	AUTO_PLAY_MUSIC_SOURCE_T music_src;
	int is_need_switch_folder;
}SD_MUSIC_MANAGE_HANDLE_T;

void sd_music_manage_create(void);
void sd_music_manage_delete(void);
void sd_music_manage_mount(void);
void sd_music_manage_unmount(void);
void sd_music_start();
int get_sd_scan_status(void);
void set_sd_scan_status(int status);
void set_sd_switch_folder_status(int status);
int get_sd_switch_folder_status(void);


#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

