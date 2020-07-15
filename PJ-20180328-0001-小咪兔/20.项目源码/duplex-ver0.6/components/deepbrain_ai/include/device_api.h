/* device_api.h
**
** Copyright (C) 2005-2009 Collecta, Inc. 
**
**  This software is provided AS-IS with no warranty, either express
**  or implied.
**
**  This program is dual licensed under the MIT and GPLv3 licenses.
*/

#ifndef __DEVICE_API_H__
#define __DEVICE_API_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

uint64_t get_time_of_day(void);
void device_get_mac(unsigned char eth_mac[6]);
void device_get_mac_str(char *_out, size_t out_len);
void *esp32_malloc(size_t _size);
void esp32_free(void *_mem);
void device_sleep(int sec, int usec);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __DEVICE_API_H__ */

