
#ifndef __CHAT_TALK_H__
#define __CHAT_TALK_H__

#include "MediaService.h"


#ifdef __cplusplus
extern "C" {
#endif /* C++ */

void chat_talk_create(MediaService *self);
void chat_talk_delete(void);
void chat_talk_start(void);
void chat_talk_stop(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

