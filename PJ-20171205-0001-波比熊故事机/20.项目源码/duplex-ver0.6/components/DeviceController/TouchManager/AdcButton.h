#ifndef _ADC_BUTTON_H_
#define _ADC_BUTTON_H_

typedef enum {
    BTN_STATE_IDLE,  // 0: idle
    BTN_STATE_CLICK, // 1: pressed
    BTN_STATE_PRESS, // 2: long pressed
    BTN_STATE_RELEASE, // 3: released
} BtnState;


typedef enum {
    USER_KEY_SET,
    USER_KEY_PLAY,
    USER_KEY_REC,
    USER_KEY_MODE,
    USER_KEY_VOL_DOWN,
    USER_KEY_VOL_UP,
    USER_KEY_MAX,
} UserKeyName;

typedef void (*ButtonCallback) (int id, BtnState state);

void adc_btn_test(void);
void adc_btn_init(ButtonCallback cb);

#endif
