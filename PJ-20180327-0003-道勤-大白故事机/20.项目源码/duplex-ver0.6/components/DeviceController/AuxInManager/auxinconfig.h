#ifndef _AUX_IN_H_
#define _AUX_IN_H_
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void auxin_intr_init(xQueueHandle queue);

int auxin_status_detect(void);

#endif
