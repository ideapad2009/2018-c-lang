
#ifndef __FREE_TALK_H__
#define __FREE_TALK_H__

#include "MediaService.h"


#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//free talk 提示音播放状态
typedef enum
{
	FREE_TALK_TONE_STATE_INIT,				//初始状态
	FREE_TALK_TONE_STATE_DISABLE_PLAY,		//不允许播放“嘟”声
	FREE_TALK_TONE_STATE_ENABLE_PLAY,		//允许播放“嘟”声
}FREE_TALK_TONE_STATE_T;

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
	FREE_TALK_STATE_TRANSLATE_EN_OR_CH,//翻译：中英自动识别模式
}FREE_TALK_STATE_T;

//启动对话时播放提示音的序号
typedef enum
{
	FREE_TALK_TONE_NUM_ONE,				//序号1
	FREE_TALK_TONE_NUM_TWO,				//序号2
	FREE_TALK_TONE_NUM_THREE,			//序号3
}FREE_TALK_TONE_NUM_T;

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

//对话前提示音是否播放接口，state为播放选项
void set_talk_tone_state(int state);
int get_talk_tone_state();

void free_talk_tone();			//使每次开启时播放的提示音不同

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

