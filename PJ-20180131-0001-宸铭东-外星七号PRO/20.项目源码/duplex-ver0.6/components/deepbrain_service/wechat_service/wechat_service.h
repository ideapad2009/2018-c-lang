#ifndef __WECHAT_SERVICE_H__
#define __WECHAT_SERVICE_H__
#include <stdio.h>
#include "asr_service.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

typedef struct
{
	xQueueHandle msg_queue;
}WECHAT_SERVICE_HANDLE_T;

typedef enum 
{
	WECHAT_MSG_TYPE_ASR_RET,
	WECHAT_MSG_TYPE_QUIT,
}WECHAT_MSG_TYPE_T;

typedef struct
{
	WECHAT_MSG_TYPE_T msg_type;
	ASR_RESULT_T asr_result;
}WECHAT_MSG_T;

//interface
void wechat_service_create(void);
void wechat_service_delete(void);
void wechat_record_start(void);
void wechat_record_stop(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

