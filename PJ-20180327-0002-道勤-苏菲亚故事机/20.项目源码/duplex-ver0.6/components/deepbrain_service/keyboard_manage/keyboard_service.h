#ifndef __KEYBOARD_SERVICE_H__
#define __KEYBOARD_SERVICE_H__
#include <stdio.h>
#include "MediaService.h"
#include "keyboard_event.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//interface
void keyboard_service_create(MediaService *self);
void keyboard_service_delete(void);
void key_matrix_callback(KEY_EVENT_T key_event);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

