#ifndef _INTERRUPTION_SAL_H_
#define _INTERRUPTION_SAL_H_

#include "esp_intr_alloc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
    Please refer to "esp-idf\components\soc\esp32\include\soc\soc.h" to learn the distribution of all the interruption.
*/

#define GPIO_INTER_FLAG     ESP_INTR_FLAG_LEVEL1
#define TOUCH_INTER_FLAG    ESP_INTR_FLAG_LEVEL2 //can not be set as 1
#define I2S_INTER_FLAG      ESP_INTR_FLAG_LEVEL2

int GpioInterInstall();


#ifdef __cplusplus
}
#endif

#endif  //_INTERRUPTION_SAL_H_
