#ifndef SEMAPHONE_LOCK_H
#define SEMAPHONE_LOCK_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define SEMPHR_CREATE_LOCK(lock) 	do{lock = xSemaphoreCreateMutex();}while(0)
#define SEMPHR_TRY_LOCK(lock) 		do{if (lock != NULL) xSemaphoreTake(lock, portMAX_DELAY);}while(0)
#define SEMPHR_TRY_UNLOCK(lock) 	do{if (lock != NULL) xSemaphoreGive(lock);}while(0)
#define SEMPHR_DELETE_LOCK(lock) 	do{if (lock != NULL) vSemaphoreDelete(lock);}while(0)

#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

#endif 

