
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

typedef enum 
{
	LED_CTRL_INITIAL = 0,		//初始状态
	LED_CTRL_EYEBROW_LIGHT,		//眉毛灯常亮
	LED_CTRL_EYEBROW_CLOSE,		//眉毛灯关闭
	LED_CTRL_EYE_LIGHT,			//眼睛灯常亮
	LED_CTRL_EYE_CLOSE,			//眼睛灯关闭
	LED_CTRL_EYE_FLASHING_SLOW,	//眼睛灯0.8秒闪烁1次
	LED_CTRL_EYE_FLASHING_FAST,	//眼睛灯0.2秒闪烁1次
	LED_CTRL_ALL_CLOSE,			//关闭所有灯光
}LED_CTRL_MODE_E;

void UserGpioInit();
void LedIndicatorSet(uint32_t num, LedWorkState state);

//设置led状态
void led_ctrl_set_mode(LED_CTRL_MODE_E mode);

void led_ctrl_service_create(void);
void led_ctrl_service_delete(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __BAIDU_ASR_API_H__ */

