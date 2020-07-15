#ifndef __BIND_DEVICE_H__
#define __BIND_DEVICE_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

typedef struct
{
	char comm_buffer[512];
}BIND_DEVICE_HANDLE_T;

void bind_device_create(void);
void bind_device_delete(void);


#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __DEVICE_API_H__ */

