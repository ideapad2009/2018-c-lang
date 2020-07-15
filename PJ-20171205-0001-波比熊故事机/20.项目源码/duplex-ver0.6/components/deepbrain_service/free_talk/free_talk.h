
#ifndef __FREE_TALK_H__
#define __FREE_TALK_H__

#include "MediaService.h"


#ifdef __cplusplus
extern "C" {
#endif /* C++ */

typedef enum
{
	FREE_TALK_MODE_INIT,			//初始状态
	FREE_TALK_MODE_KEYWORD_WAKEUP,	//唤醒词唤醒
	FREE_TALK_MODE_VAD_WAKEUP,		//静音检测唤醒
	FREE_TALK_MODE_SPEECH_DETECT,	//检测说话
	FREE_TALK_MODE_SPEECH_END,		//检测说话
	FREE_TALK_MODE_STOP,			//停止
}FREE_TALK_MODE_T;

typedef enum
{
	FREE_TALK_STATE_FLAG_INIT_STATE,		//初始状态
	FREE_TALK_STATE_FLAG_START_STATE,		//开启状态标记
	FREE_TALK_STATE_FLAG_STOP_STATE			//停止状态标记
}FREE_TALK_STATE_FLAG_T;

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
void free_talk_set_mode(int _mode);
int free_talk_get_mode(void);
void set_free_talk_flag(int free_talk_flag);
int get_free_talk_flag();
void set_free_talk_pause(void);
void set_free_talk_resume(void);
void free_talk_start_after_music(void);
void free_talk_stop_before_music(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

