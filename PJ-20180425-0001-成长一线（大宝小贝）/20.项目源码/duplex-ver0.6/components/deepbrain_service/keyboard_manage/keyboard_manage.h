#ifndef _KEYBOARD_MATRIX_H_
#define _KEYBOARD_MATRIX_H_

#include "driver/gpio.h"
#include "userconfig.h"
#include "keyboard_event.h"

//key event enum
typedef enum  
{
	DEVICE_KEY_START = 0,
		
	DEVICE_KEY_K1_TAP,
	DEVICE_KEY_K1_PUSH,
	DEVICE_KEY_K1_RELEASE,
	DEVICE_KEY_K1_LONGPRESSED,

	DEVICE_KEY_K2_TAP,
	DEVICE_KEY_K2_PUSH,
	DEVICE_KEY_K2_RELEASE,
	DEVICE_KEY_K2_LONGPRESSED,

	DEVICE_KEY_K3_TAP,
	DEVICE_KEY_K3_PUSH,
	DEVICE_KEY_K3_RELEASE,
	DEVICE_KEY_K3_LONGPRESSED,

	DEVICE_KEY_K4_TAP,
	DEVICE_KEY_K4_PUSH,
	DEVICE_KEY_K4_RELEASE,
	DEVICE_KEY_K4_LONGPRESSED,

	DEVICE_KEY_K5_TAP,
	DEVICE_KEY_K5_PUSH,
	DEVICE_KEY_K5_RELEASE,
	DEVICE_KEY_K5_LONGPRESSED,

	DEVICE_KEY_K6_TAP,
	DEVICE_KEY_K6_PUSH,
	DEVICE_KEY_K6_RELEASE,
	DEVICE_KEY_K6_LONGPRESSED,

	DEVICE_KEY_K7_TAP,
	DEVICE_KEY_K7_PUSH,
	DEVICE_KEY_K7_RELEASE,
	DEVICE_KEY_K7_LONGPRESSED,
	
	DEVICE_KEY_END,
}DEVICE_KEY_EVENT_T;

typedef enum {
	KEY_EVENT_IDEL = 0,
	KEY_EVENT_CHAT_START,    	//1  语聊
	KEY_EVENT_CHAT_STOP,     	//2
	KEY_EVENT_WECHAT_START,  	//3  微聊
	KEY_EVENT_WECHAT_STOP,   	//4
	KEY_EVENT_VOL_UP,        	//5  音量加
	KEY_EVENT_VOL_DOWN,	     	//6  音量减
	KEY_EVENT_PREV,          	//7  上一首
	KEY_EVENT_NEXT,          	//8  下一首
	KEY_EVENT_PUSH_PLAY,     	//9  暂停、播放
	KEY_EVENT_MENU,          	//10 菜单
	KEY_EVENT_WIFI,          	//11 WIFI
	KEY_EVENT_SDCARD,        	//12 SD卡
	KEY_EVENT_FLASH,         	//13 Flash
	KEY_EVENT_LED_OPEN,      	//14 LED open
	KEY_EVENT_LED_CLOSE,     	//15 LED close
	KEY_EVENT_COLLECT,       	//16 收藏
	KEY_EVENT_PLAY_COLLECT,  	//17 播放收藏
	KEY_EVENT_CHILD_LOCK,    	//18 童锁
	KEY_EVENT_OTHER_KEY_CHILD_LOCK_PLAY,
	KEY_EVENT_SLEEP,         	//19 睡眠
	KEY_EVENT_SLEEP_STORY,   	//20 睡前故事
	KEY_EVENT_TEST_UPDATE,   	//21 测试升级
	KEY_EVENT_STANDBY,       	//22 待机
	KEY_EVENT_HEAD_LED_SWITCH,	//23 头灯开关
	KEY_EVENT_TRANSLATE,		//24 中英互译
	/* Add event here*/
}KEY_EVENT_T;
typedef void (* key_matrix_cb)(DEVICE_KEY_EVENT_T key_event);
typedef void (* battery_cb)(float _battery_vol);
typedef void* key_matrix_handle_t;

key_matrix_handle_t adc_keyboard_matrix_init(key_matrix_cb _cb, battery_cb bat_cb);
void adc_keyboard_matrix_free(key_matrix_handle_t _handle);

#endif

