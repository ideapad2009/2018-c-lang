
#ifndef __CHAT_TALK_H__
#define __CHAT_TALK_H__

#include "MediaService.h"
#include "asr_service.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

typedef enum 
{
	CHAT_TALK_MSG_TYPE_ASR_RET,
	CHAT_TALK_MSG_TYPE_QUIT,
}CHAT_TALK_MSG_TYPE_T;

typedef struct
{
	CHAT_TALK_MSG_TYPE_T msg_type;
	ASR_RESULT_T asr_result;
}CHAT_TALK_MSG_T;

typedef struct
{
	xQueueHandle msg_queue;
	int cur_play_index;
	NLP_RESULT_T nlp_result;
	MediaService *service;
}CHAT_TALK_HANDLE_T;

void chat_talk_create(MediaService *self);
void chat_talk_delete(void);
void chat_talk_start(void);
void chat_talk_stop(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

