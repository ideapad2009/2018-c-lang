#ifndef __TRANSLATE_MANAGE_H__
#define __TRANSLATE_MANAGE_H__

#include "MediaService.h"
#include "nlp_service.h"
#include "player_middleware.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//#define DEBUG_PRINT
#define TRANSLATE_TAG							"translate_manage"
#define TRANSLATE_TALK_AMR_MAX_TIME				10
#define TRANSLATE_TALK_MAX_TIME_MS				(TRANSLATE_TALK_AMR_MAX_TIME*1000)
#define TRANSLATE_TALK_MAX_AMR_AUDIO_SIZE 		(TRANSLATE_TALK_MAX_TIME_MS*AMR_20MS_AUDIO_SIZE/20 + 6)
#define TRANSLATE_SEND_MSG_URL 	 				"http://api.deepbrain.ai:8383/open-api/service"	//翻译接口
#define TRANSLATE_TEXT_SIZE						128

//翻译功能的返回值
typedef enum
{
	TRANSLATE_MANAGE_RET_INIT,				// 返回值为初始状态
	TRANSLATE_MANAGE_RET_GET_RESULT_OK,		// 执行成功
	TRANSLATE_MANAGE_RET_GET_RESULT_FAIL,	// 执行失败
}TRANSLATE_MANAGE_RET_T;

typedef enum
{
	TRANSLATE_TALK_MSG_RECORD_START,
	TRANSLATE_TALK_MSG_RECORD_READ,
	TRANSLATE_TALK_MSG_RECORD_STOP,
	TRANSLATE_TALK_MSG_QUIT,
}TRANSLATE_TALK_MSG_TYPE_T;

typedef struct
{
	int record_quiet_ms;
	int record_ms;
	int keyword_record_ms;
	int record_len;
	char record_data[TRANSLATE_TALK_MAX_AMR_AUDIO_SIZE];
}TRANSLATE_TALK_STATUS_T;

typedef struct
{
	int data_len;
	char data[AUDIO_RECORD_BUFFER_SIZE];
}TRANSLATE_TALK_ENCODE_BUFF_T;

typedef struct 
{
	int record_id;
	int record_len;
	int record_ms;
	int is_max_ms;
	char data[AUDIO_RECORD_BUFFER_SIZE];
}TRANSLATE_TALK_RECORD_DATA_T;

typedef struct
{
	char asr_result[512];
}TRANSLATE_TALK_ASR_RESULT_T;

typedef struct
{
	xQueueHandle msg_queue;
	TRANSLATE_TALK_STATUS_T status;
	void *amr_encode_handle;
	TRANSLATE_TALK_ENCODE_BUFF_T pcm_encode;
	char str_comm_buf[BUFFER_SIZE_1024*2];				// commucation buffer
	NLP_RESULT_T nlp_result;
	MediaService *service;
	BAIDU_ACOUNT_T baidu_acount;
	TRANSLATE_TALK_ASR_RESULT_T asr_result;
	char translate_result_text[TRANSLATE_TEXT_SIZE];
}TRANSLATE_TALK_HANDLE_T;

typedef struct
{
	TRANSLATE_TALK_MSG_TYPE_T msg_type;
	TRANSLATE_TALK_RECORD_DATA_T *p_record_data;
}TRANSLATE_TALK_MSG_T;

void translate_talk_create(MediaService *self);
void translate_talk_delete(void);
void translate_talk_start(void);
void translate_talk_stop(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif
