#ifndef __TRANSLATE_MANAGE_H__
#define __TRANSLATE_MANAGE_H__

#include "MediaService.h"
#include "nlp_service.h"
#include "player_middleware.h"
#include "asr_service.h"

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
	TRANSLATE_MANAGE_MODE_FLAG_INIT,			//	初始模式
	TRANSLATE_MANAGE_MODE_FLAG_EN_AND_CH,		//	中英互译模式
}TRANSLATE_MANAGE_MODE_FLAG_T;

typedef enum
{
	TRANSLATE_TONE_NUM_ONE,				//序号1
	TRANSLATE_TONE_NUM_TWO,				//序号2
}TRANSLATE_TONE_NUM_T;

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

typedef enum 
{
	TRANSLATE_MSG_TYPE_ASR_RET,
	TRANSLATE_MSG_TYPE_QUIT,
}TRANSLATE_MSG_TYPE_T;

typedef struct
{
	xQueueHandle msg_queue;
}TRANSLATE_QUEUE_HANDLE_T;

typedef struct
{
	TRANSLATE_MSG_TYPE_T msg_type;
	ASR_RESULT_T asr_result;
}TRANSLATE_MSG_T;

typedef struct
{
	char 	str_type[8];
	char	domain[64];
	char	port[6];
	char	params[128];
	char	str_nonce[64];
	char	str_timestamp[64];
	char	str_private_key[64];
	char	str_device_id[32];
	char	str_buf[512];
	char 	str_comm_buf[2048];
}TRANSLATE_HTTP_BUFFER;

#if 0
void translate_service_create(void);
void translate_service_delete(void);
void translate_record_start(void);
void translate_record_stop(void);
void translate_asr_result_cb(ASR_RESULT_T *result);
#endif
void set_translate_mode_flag(int translate_mode_flag);
int get_translate_mode_flag();
void translate_process(char *text);
void translate_tone();

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif
