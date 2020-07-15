/*
#ifndef _LED_H_
#define _LED_H_
typedef enum {
    PwmChannel0 = 0,
    PwmChannel1,
    PwmChannel2,
    PwmChannel3,
    PwmChannel4,
    PwmChannel5,
    PwmChannel6,
    PwmChannel7,
} PwmChannel;   //PwmChannel;

#define LedIndexNet         PwmChannel0
#define LedIndexSys         PwmChannel1

typedef enum {
    LedWorkState_NetSetting,    // Green led ms-on,ms-off
    LedWorkState_NetConnectOk,  // Green led on
    LedWorkState_NetDisconnect, // Green led on

    LedWorkState_SysBufLow,     // Red led s-On,s-Off;
    LedWorkState_SysPause,      // Red led on
    LedWorkState_Off,           // Turn off the led
    LedWorkState_Unknown,       //
} LedWorkState;

void UserGpioInit();
void LedIndicatorSet(uint32_t num, LedWorkState state);

//眉毛灯和眼睛灯 状态接口
#if MODULE_EXTRATERRESTRIAL_SEVEN_CASE
void change_eyebrow_led_state(void);
void change_eye_led_state(void);

//眉毛灯常亮
void eyebrow_led_light();

//眉毛灯灭
void eyebrow_led_close();

//眼睛灯常亮
void eye_led_light();

//眼睛灯灭
void eye_led_close();

//眼睛灯慢速闪动（1秒1次）
void eye_led_slow_flashing();

//眼睛灯快速闪动（1秒4次）
void eye_led_fast_flashing();
#endif

#endif
*/
