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
	KEY_EVENT_CHAT_START,    	//1  ����
	KEY_EVENT_CHAT_STOP,     	//2
	KEY_EVENT_WECHAT_START,  	//3  ΢��
	KEY_EVENT_WECHAT_STOP,   	//4
	KEY_EVENT_VOL_UP,        	//5  ������
	KEY_EVENT_VOL_DOWN,	     	//6  ������
	KEY_EVENT_PREV,          	//7  ��һ��
	KEY_EVENT_NEXT,          	//8  ��һ��
	KEY_EVENT_PUSH_PLAY,     	//9  ��ͣ������
	KEY_EVENT_MENU,          	//10 �˵�
	KEY_EVENT_WIFI,          	//11 WIFI
	KEY_EVENT_SDCARD,        	//12 SD��
	KEY_EVENT_FLASH,         	//13 Flash
	KEY_EVENT_LED_OPEN,      	//14 LED open
	KEY_EVENT_LED_CLOSE,     	//15 LED close
	KEY_EVENT_COLLECT,       	//16 �ղ�
	KEY_EVENT_PLAY_COLLECT,  	//17 �����ղ�
	KEY_EVENT_CHILD_LOCK,    	//18 ͯ��
	KEY_EVENT_SLEEP,         	//19 ˯��
	KEY_EVENT_SLEEP_STORY,   	//20 ˯ǰ����
	KEY_EVENT_TEST_UPDATE,   	//21 ��������
	KEY_EVENT_STANDBY,       	//22 ����
	KEY_EVENT_HEAD_LED_SWITCH,	//23 ͷ�ƿ���
	KEY_EVENT_TRANSLATE,		//24 ��Ӣ����
	KEY_EVENT_CH2EN,			//中译英
	KEY_EVENT_EN2CH,			//英译中
	/* Add event here*/
}KEY_EVENT_T;
typedef void (* key_matrix_cb)(DEVICE_KEY_EVENT_T key_event);
typedef void (* battery_cb)(float _battery_vol);
typedef void* key_matrix_handle_t;

key_matrix_handle_t adc_keyboard_matrix_init(key_matrix_cb _cb, battery_cb bat_cb);
void adc_keyboard_matrix_free(key_matrix_handle_t _handle);

#endif

