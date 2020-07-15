#ifndef __FUNCTIONAL_RUNNING_STATE_CHECK_H__
#define __FUNCTIONAL_RUNNING_STATE_CHECK_H__

/******************* 按键功能状态 set&get,使一些功能互斥；**********************/
typedef enum functional_trigger_key
{
	FUNCTIONAL_TYIGGER_KEY_INITIAL_STATE,					//初始状态
	FUNCTIONAL_TYIGGER_KEY_SAVE_PLAY_STATE,					//收藏键播放状态
	FUNCTIONAL_TYIGGER_KEY_GUOXUE_STATE,					//国学键状态
	FUNCTIONAL_TYIGGER_KEY_FLASH_SD_STATE,					//本地音频键状态
	FUNCTIONAL_TYIGGER_KEY_SPEECH_RECOGNITION_STATE,		//对话键状态
	FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE,		//童锁键状态
	FUNCTIONAL_TYIGGER_KEY_CHAT_STATE,						//微聊键状态
	FUNCTIONAL_TYIGGER_KEY_SET_WIFI_STATE,					//配网键状态
	FUNCTIONAL_TYIGGER_KEY_OTA_STATE,						//OTA升级状态
	FUNCTIONAL_TYIGGER_KEY_BEDTIME_STORY_STATE				//播放睡前故事状态
}FUNCTIONAL_TYIGGER_KEY_E;
	
void set_functional_tyigger_key_state(FUNCTIONAL_TYIGGER_KEY_E key_state);
FUNCTIONAL_TYIGGER_KEY_E get_functional_tyigger_key_state();

/*************************************  网络状态 set&get      *************************************/
typedef enum wifi_state
{
	WIFI_INITIAL_STATE,		//初始状态
	WIFI_OFF_STATE,			//断网状态
	WIFI_ON_STATE,			//已联网状态
}WIFI_STATE_E;
	
void set_wifi_state(WIFI_STATE_E wifi_state);
WIFI_STATE_E get_wifi_state();

#endif
