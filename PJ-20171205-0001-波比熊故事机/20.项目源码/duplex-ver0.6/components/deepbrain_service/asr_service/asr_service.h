
#ifndef __ASR_SERVICE_H__
#define __ASR_SERVICE_H__
#include <stdio.h>
#include "player_middleware.h"
#include "amrwb_encode.h"
#include "unisound_api.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define AMRNB_HEADER "#!AMR\n"
#define AMR_ENCODE_FRAME_MS	20
#define AMRNB_ENCODE_IN_BUFF_SIZE 320
#define AMRNB_ENCODE_OUT_BUFF_SIZE (AMRNB_ENCODE_IN_BUFF_SIZE/10)
#define ASR_AMR_MAX_TIME_SEC	10
#define ASR_AMR_MAX_TIME_MS		(ASR_AMR_MAX_TIME_SEC*1000)
#define ASR_MAX_AMR_AUDIO_SIZE 	(ASR_AMR_MAX_TIME_MS*AMRWB_ENC_OUT_BUFFER_SZ/20 + 9)
#define ASR_MIN_AUDIO_MS	1000

//语音识别结果类型
typedef enum
{
	ASR_RESULT_TYPE_SHORT_AUDIO,
	ASR_RESULT_TYPE_FAIL,
	ASR_RESULT_TYPE_SUCCESS,
	ASR_RESULT_TYPE_NO_RESULT,
}ASR_RESULT_TYPE_T;

//语音识别结果
typedef struct 
{
	ASR_RESULT_TYPE_T type;
	char asr_text[300];//100个汉子
	char record_data[RAW_PCM_LEN_S(10, 16000, 2)/5];
	uint32_t record_ms;
	uint32_t record_len;
}ASR_RESULT_T;

//语音识别录音类型
typedef enum
{
	ASR_SERVICE_RECORD_START,
	ASR_SERVICE_RECORD_READ,
	ASR_SERVICE_RECORD_STOP,
	ASR_SERVICE_QUIT,
}ASR_MSG_TYPE_T;

//语音识别输入音频类型
typedef enum
{
	//amr
	ASR_RECORD_TYPE_AMRNB,
	ASR_RECORD_TYPE_AMRWB,
	//speex
	ASR_RECORD_TYPE_SPXNB,
	ASR_RECORD_TYPE_SPXWB,
	//pcm
	ASR_RECORD_TYPE_PCM_UNISOUND_16K,
	ASR_RECORD_TYPE_PCM_SINOVOICE_16K,
}ASR_RECORD_TYPE_T;

//语音识别输入PCM数据
typedef struct 
{
	int record_id;
	int record_len;
	int record_ms;
	int is_max_ms;
	uint64_t time_stamp;
	ASR_RECORD_TYPE_T record_type;
	char record_data[RAW_PCM_LEN_MS(200, 16000, 2)];
}ASR_RECORD_DATA_T;

//语音识别结果回调
typedef void (*asr_service_result_cb)(ASR_RESULT_T *result);

//语音识别输入数据消息
typedef struct
{
	ASR_MSG_TYPE_T msg_type;
	ASR_RECORD_DATA_T record_data;
	asr_service_result_cb call_back;
}ASR_RECORD_MSG_T;

typedef struct
{
	char 	 record_data_8k[RAW_PCM_LEN_MS(200, 8000, 2)];
	uint32_t record_data_8k_len;
}AMRNB_ENCODE_BUFF_T;

typedef struct ASR_CONTEXT ASR_CONTEXT_T;

typedef struct
{
	xQueueHandle msg_queue;
	void *amrnb_encode;
	void *amrwb_encode;
	ASR_RESULT_T asr_result;
	BAIDU_ACOUNT_T baidu_acount;
	SINOVOICE_ACOUNT_T sinovoice_acount;
	UNISOUND_ASR_ACCOUNT_T unisound_acount;
	AMRNB_ENCODE_BUFF_T amrnb_buff;
	ASR_CONTEXT_T *asr_ctx;
	void *unisound_asr_handle;
	void *sinovoice_asr_handle;
}ASR_SERVICE_HANDLE_T;

//interface
void asr_service_create(void);
void asr_service_delete(void);
int asr_service_send_request(ASR_RECORD_MSG_T *msg);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

