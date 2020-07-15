#ifndef __PLAYER_MIDDLERWARE_H__
#define __PLAYER_MIDDLERWARE_H__

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "array_music.h"

#define AMR_HEADER "#!AMR\n"
#define AMR_20MS_AUDIO_SIZE 32
#define AUDIO_PCM_RATE	16000
#define AUDIO_PCM_BITE	16
#define AUDIO_PCM_FRAME_MS 200
#define AUDIO_PCM_ONE_MS_BYTE ((AUDIO_PCM_RATE/1000)*(AUDIO_PCM_BITE/8))
#define AUDIO_RECORD_BUFFER_SIZE (AUDIO_PCM_ONE_MS_BYTE*AUDIO_PCM_FRAME_MS)
#define AUDIO_PCM_10MS_FRAME_SIZE (10*AUDIO_PCM_ONE_MS_BYTE) 
#define GET_AUDIO_PCM_MS(len) (len/AUDIO_PCM_ONE_MS_BYTE)

typedef enum 
{
	PLAYER_MDWARE_STATUS_IDEL = 0,
	PLAYER_MDWARE_STATUS_PLAY_TONE,
	PLAYER_MDWARE_STATUS_PLAY_MUSIC,
	PLAYER_MDWARE_STATUS_RECORD_START,
	PLAYER_MDWARE_STATUS_RECORD_STOP,
}PLAYER_MDWARE_STATUS_T;

typedef enum
{
	PLAYER_MDWARE_MSG_NONE,
	PLAYER_MDWARE_MSG_PLAY_TONE,
	PLAYER_MDWARE_MSG_STOP_TONE,
	PLAYER_MDWARE_MSG_PLAY_MUSIC,
	PLAYER_MDWARE_MSG_RECORD_START,
	PLAYER_MDWARE_MSG_RECORD_STOP,
}PLAYER_MDWARE_MSG_TYPE_T;

typedef void (*player_mdware_record_cb)(
	int id, char *data, size_t data_len, int record_ms, int is_max);

//录音消息
typedef struct 
{
	int record_id;
	int record_ms;
	player_mdware_record_cb cb;
}PLAYER_MDWARE_MSG_START_RECORD_T;

//播放flash music tone
typedef struct
{
	int audio_len;
	char *audio_data;
	int is_translate;//是否是翻译结果
}PLAYER_MDWARE_MSG_PLAY_TONE_T;

//播放器中间件消息
typedef struct
{
	PLAYER_MDWARE_MSG_TYPE_T msg_type;
	PLAYER_MDWARE_MSG_START_RECORD_T msg_start_recrod;
	PLAYER_MDWARE_MSG_PLAY_TONE_T msg_play_tone;
}PLAYER_MDWARE_MSG_T;

typedef struct
{
	PLAYER_MDWARE_STATUS_T cur_status;
	PLAYER_MDWARE_STATUS_T last_status;
	xQueueHandle queue_player_msg;
	xQueueHandle queue_record_msg;
	xQueueHandle queue_play_tone_msg;
}PLAYER_MDWARE_CONFIG_T;

void create_player_middleware(void);
void player_mdware_start_record(
	int record_id, 
	int record_ms,
	player_mdware_record_cb cb);
void player_mdware_stop_record(void);
void player_mdware_set_translate_result(char *data, size_t data_len);
void player_mdware_play_translate_result(void);
void player_mdware_play_tone(FLASH_MUSIC_INDEX_T flash_music_index);

#endif
