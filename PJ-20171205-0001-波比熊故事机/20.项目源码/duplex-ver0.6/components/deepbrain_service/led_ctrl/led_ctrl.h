
#ifndef __LED_CTRL_H__
#define __LED_CTRL_H__

#include "ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define LedIndexNet         PwmChannel0
#define LedIndexSys         PwmChannel1

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
void switch_color_led(void);
void color_led_on();
void color_led_off();
void color_led_flicker_stop(void);
void color_led_flicker_start();

#endif
