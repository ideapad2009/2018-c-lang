#ifndef __WECHAT_SERVICE_H__
#define __WECHAT_SERVICE_H__
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//interface
void wechat_service_create(void);
void wechat_service_delete(void);
void wechat_record_start(void);
void wechat_record_stop(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

