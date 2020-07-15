
#ifndef _DEEP_BRAIN_SERVICE_H_
#define _DEEP_BRAIN_SERVICE_H_
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "MediaService.h"

typedef struct DeepBrainService { //extern from TreeUtility
    /*relation*/
    MediaService Based;
    /*private*/
} DeepBrainService;

DeepBrainService *DeepBrainCreate();
void db_set_record_id(unsigned int _id);
unsigned int db_get_record_id(void);


#endif

