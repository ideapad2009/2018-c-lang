#ifndef _SD_CARD_UTIL_H_
#define _SD_CARD_UTIL_H_
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

esp_err_t sd_card_mount(const char* basePath);

esp_err_t sd_card_unmount(void);

int sd_card_status_detect(void);
int sd_card_insert_status(void);

void sd_card_intr_init(xQueueHandle queue);
#endif
