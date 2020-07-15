#ifndef _MPUSH_SERVICE_H_
#define _MPUSH_SERVICE_H_
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "MediaService.h"
#include "mpush_client_api.h"
#include "nlp_service.h"

struct AudioTrack;

typedef struct
{
    /*relation*/
    MediaService Based;
    /*private*/
    QueueHandle_t _OtaQueue;
	void *_mpush_handler;
} MPUSH_SERVICE_T;

typedef struct
{
	int cur_play_index;			//当前播放的索引
	NLP_RESULT_T nlp_result;	//nlp返回结果
}APP_ASK_HANDLE_T;

MPUSH_SERVICE_T *mpush_service_create(void);

int mpush_service_send_text(
	const char * _string, 
	const char * _nlp_text,
	const char *_to_user);

int mpush_service_send_file(
	const char * _data, 
	const size_t data_len, 
	const char * _nlp_text,
	const char *_file_type,
	const char *_to_user);


#endif
