#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_types.h"
#include "esp_log.h"

#include "auxinconfig.h"
#include "userconfig.h"

#define TIME                1000
#define AUXIN_TAG           "AUXIN_CONFIG"

static xTimerHandle TimerRec;
static xTimerHandle TimerMode;
static xTimerHandle TimerAux;

static void timer_cb(TimerHandle_t xTimer)
{
    if(xTimer == TimerRec)
        ets_printf("TimerRec Timer Callback\n");
    else if(xTimer == TimerMode)
        ets_printf("TimerMode Timer Callback\n");
    else if(xTimer == TimerAux)
        ets_printf("TimerMode Timer Callback\n");
}

static int timer_init()
{
    TimerRec = xTimerCreate("timer0", TIME / portTICK_RATE_MS, pdFALSE, (void*) 0, timer_cb);
    TimerMode = xTimerCreate("timer1", TIME / portTICK_RATE_MS, pdFALSE, (void*) 0, timer_cb);
    TimerAux = xTimerCreate("timer1", TIME / portTICK_RATE_MS, pdFALSE, (void*) 0, timer_cb);
    if (!TimerRec || !TimerMode || !TimerAux) {
        ESP_LOGE(AUXIN_TAG, "timer create err\n");
        return -1;
    }
    xTimerStart(TimerRec, TIME / portTICK_RATE_MS);
    xTimerStart(TimerMode, TIME / portTICK_RATE_MS);
    xTimerStart(TimerAux, TIME / portTICK_RATE_MS);

    return 0;
}

static void timer_delete()
{
    xTimerDelete(TimerRec, TIME / portTICK_RATE_MS);
    TimerRec = NULL;
    xTimerDelete(TimerMode, TIME / portTICK_RATE_MS);
    TimerMode = NULL;
    xTimerDelete(TimerAux, TIME / portTICK_RATE_MS);
    TimerAux = NULL;
}

static void IRAM_ATTR auxin_gpio_intr_handler(void* arg)
{
    xQueueHandle queue = (xQueueHandle) arg;
    gpio_num_t gpioNum = (gpio_num_t) GPIO_AUX;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (queue)
        xQueueSendFromISR(queue, &gpioNum, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }

//    if(gpioNum == GPIO_REC)
//        xTimerResetFromISR(TimerRec, TIME / portTICK_RATE_MS);
//    else if(gpioNum == GPIO_MODE)
//        xTimerResetFromISR(TimerMode, TIME / portTICK_RATE_MS);
//     else if(gpioNum == GPIO_AUX)
//        xTimerResetFromISR(TimerAux, TIME / portTICK_RATE_MS);
}

int auxin_status_detect()
{
    vTaskDelay(1000 / portTICK_RATE_MS);
    return gpio_get_level(GPIO_AUX);
}

void auxin_intr_init(xQueueHandle queue)
{
    //timer
//    timer_init();

    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    //Aux_in
#if IDF_3_0
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
#else
    io_conf.intr_type = GPIO_PIN_INTR_ANYEGDE;
#endif
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_SEL_AUX;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    gpio_isr_handler_add(GPIO_AUX, auxin_gpio_intr_handler, (void*) queue);
}

