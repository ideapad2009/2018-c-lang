
#ifndef __ASR_SERVICE_H__
#define __ASR_SERVICE_H__
#include <stdio.h>
#include "player_middleware.h"
#include "amrwb_encode.h"
#include "dp_comm_library_interface.h"
#include "flash_config_manage.h"

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
#define ASR_SINGLE_FRAME_RECORD_MS AUDIO_PCM_FRAME_MS

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
	NLP_RESULT_T nlp_result;
	BAIDU_ACOUNT_T baidu_acount;
	AMRNB_ENCODE_BUFF_T amrnb_buff;
	ASR_LANGUAGE_TYPE_T language_type;
	ASR_CONTEXT_T *asr_ctx;
	void *unisound_asr_handle;
	void *sinovoice_asr_handle;
	char result_buffer[32*1024];
}ASR_SERVICE_HANDLE_T;

//interface
void asr_service_create(void);
void asr_service_delete(void);
int asr_service_send_request(ASR_RECORD_MSG_T *msg);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

