
#ifndef __FREE_TALK_H__
#define __FREE_TALK_H__

#include "MediaService.h"


#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//free talk 运行状态
typedef enum
{
	FREE_TALK_RUN_STATUS_STOP = 0,
	FREE_TALK_RUN_STATUS_START,
}FREE_TALK_RUN_STATUS_T;

//当前自由对话状态
typedef enum 
{
	FREE_TALK_STATE_NORMAL,				//正常模式
	FREE_TALK_STATE_TRANSLATE_CH_EN,	//翻译：中译英模式
	FREE_TALK_STATE_TRANSLATE_EN_CH,	//翻译：英译中模式
}FREE_TALK_STATE_T;

void free_talk_create(MediaService *self);
void free_talk_delete(void);
void free_talk_start(void);
void free_talk_stop(void);

void free_talk_set_state(int state);
int free_talk_get_state(void);
int get_free_talk_status(void);
void set_free_talk_status(int status);
void free_talk_autostart_enable(void);
void free_talk_autostart_disable(void);
void free_talk_auto_start(void);
void free_talk_auto_stop(void);
int is_free_talk_running(void);


#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

