/* debug_log.h
** debug log api
**
** Copyright (C) 2005-2009 Collecta, Inc. 
**
**  This software is provided AS-IS with no warranty, either express
**  or implied.
**
**  This program is dual licensed under the MIT and GPLv3 licenses.
*/

#ifndef __DEBUG_LOG_H__
#define __DEBUG_LOG_H__

#include <stdint.h>
#include <stdarg.h>
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define DEBUG_LOGE( tag, format, ... ) ESP_LOGE( tag, format, ##__VA_ARGS__ )  
#define DEBUG_LOGW( tag, format, ... ) ESP_LOGW( tag, format, ##__VA_ARGS__ )  
#define DEBUG_LOGI( tag, format, ... ) ESP_LOGI( tag, format, ##__VA_ARGS__ ) 
#define DEBUG_LOGD( tag, format, ... ) ESP_LOGD( tag, format, ##__VA_ARGS__ )  
#define DEBUG_LOGV( tag, format, ... ) ESP_LOGV( tag, format, ##__VA_ARGS__ ) 

void debug_log_2_file(char *_str_content);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __DEBUG_LOG_H__ */
