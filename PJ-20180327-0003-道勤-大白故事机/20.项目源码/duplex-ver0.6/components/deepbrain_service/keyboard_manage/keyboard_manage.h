#ifndef _KEYBOARD_MATRIX_H_
#define _KEYBOARD_MATRIX_H_

#include "driver/gpio.h"
#include "userconfig.h"

//key event enum
typedef enum  
{
	DEVICE_KEY_START = 0,
		
	DEVICE_KEY_CHAT_TAP,
	DEVICE_KEY_CHAT_PUSH,
	DEVICE_KEY_CHAT_RELEASE,
	DEVICE_KEY_CHAT_LONGPRESSED,

	DEVICE_KEY_PREV_TAP,
	DEVICE_KEY_PREV_PUSH,
	DEVICE_KEY_PREV_RELEASE,
	DEVICE_KEY_PREV_LONGPRESSED,

	DEVICE_KEY_PLAY_TAP,
	DEVICE_KEY_PLAY_PUSH,
	DEVICE_KEY_PLAY_RELEASE,
	DEVICE_KEY_PLAY_LONGPRESSED,

	DEVICE_KEY_NEXT_TAP,
	DEVICE_KEY_NEXT_PUSH,
	DEVICE_KEY_NEXT_RELEASE,
	DEVICE_KEY_NEXT_LONGPRESSED,

	DEVICE_KEY_WECHAT_TAP,
	DEVICE_KEY_WECHAT_PUSH,
	DEVICE_KEY_WECHAT_RELEASE,
	DEVICE_KEY_WECHAT_LONGPRESSED,
	
	DEVICE_KEY_END,
}DEVICE_KEY_EVENT_T;

typedef void (* key_matrix_cb)(DEVICE_KEY_EVENT_T key_event);
typedef void (* battery_cb)(float _battery_vol);
typedef void* key_matrix_handle_t;

key_matrix_handle_t adc_keyboard_matrix_init(key_matrix_cb _cb, battery_cb bat_cb);
void adc_keyboard_matrix_free(key_matrix_handle_t _handle);

#endif

