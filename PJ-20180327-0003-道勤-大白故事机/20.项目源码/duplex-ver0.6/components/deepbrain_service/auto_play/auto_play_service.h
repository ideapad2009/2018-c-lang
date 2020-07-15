#ifndef __AUTO_PLAY_SERVICE_H__
#define __AUTO_PLAY_SERVICE_H__
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//interface
void auto_play_service_create(void);
void auto_play_service_delete(void);

typedef struct 
{
	int need_play_name;//是否播报歌曲名称
	char music_name[64];
	char music_url[512];
}AUTO_PLAY_MUSIC_SOURCE_T;

typedef void (*auto_play_prev_cb)(AUTO_PLAY_MUSIC_SOURCE_T *src);
typedef void (*auto_play_next_cb)(AUTO_PLAY_MUSIC_SOURCE_T *src);
typedef void (*auto_play_now_cb)(AUTO_PLAY_MUSIC_SOURCE_T *src);

typedef struct
{
	auto_play_prev_cb prev;
	auto_play_next_cb next;
	auto_play_now_cb now;
}AUTO_PLAY_CALL_BACK_T;

void play_tts(char *str_tts);
void set_auto_play_cb(AUTO_PLAY_CALL_BACK_T *call_back_list);
void get_auto_play_cb(AUTO_PLAY_CALL_BACK_T *call_back_list);
void auto_play_service_create(void);
void auto_play_service_delete(void);
void auto_play_prev();
void auto_play_next();
void auto_play_pause();
void auto_play_stop();
void auto_play_pause_resume();


#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

