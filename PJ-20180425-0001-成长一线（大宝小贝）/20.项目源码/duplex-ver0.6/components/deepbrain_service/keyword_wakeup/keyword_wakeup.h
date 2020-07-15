#ifndef KEYWORD_WAKEUP_H
#define KEYWORD_WAKEUP_H

#include "MediaService.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define KWW_AUTH_ADDR 0x731000 //唤醒库认证文件存储地址 

//唤醒通知消息
typedef enum 
{
	KWW_EVENT_WAKEUP,
}KWW_EVENT_T;

//消息通知回调函数
typedef void (*keyword_wakeup_event_cb)(KWW_EVENT_T event);

void keyword_wakeup_create(MediaService *self);
void keyword_wakeup_delete(void);
void keyword_wakeup_start(void);
void keyword_wakeup_stop(void);
int keyword_wakeup_register_callback(keyword_wakeup_event_cb cb);
int keyword_wakeup_cancel_callback(keyword_wakeup_event_cb cb);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

