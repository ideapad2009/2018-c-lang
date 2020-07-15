#ifndef __KEYBOARD_SERVICE_H__
#define __KEYBOARD_SERVICE_H__
#include <stdio.h>
#include "MediaService.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//interface
void keyboard_service_create(MediaService *self);
void keyboard_service_delete(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

