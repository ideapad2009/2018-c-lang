#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#include "esp_log.h"
#include "driver/touch_pad.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "touchpad.h"
#include "TouchManager.h"
#include "NotifyHelper.h"
#include "DeviceCommon.h"
#include "userconfig.h"
#include "button.h"
#include "AdcButton.h"

#define TOUCH_TAG "TOUCH_MANAGER"

#define TOUCHPAD_THRESHOLD                      400
#define TOUCHPAD_FILTER_VALUE                   150

#define TOUCH_EVT_QUEUE_LEN                     8
#define TOUCH_MANAGER_TASK_PRIORITY             1
#define TOUCH_MANAGER_TASK_STACK_SIZE           (3096 + 512)

static int HighWaterLine = 0;
TaskHandle_t TouchManageHandle;

#define BUTTON_ACTIVE_LEVEL   0

static QueueHandle_t xQueueTouch;
static void ProcessTouchEvent(TouchManager *manager, void *evt)
{
    touchpad_msg_t *recv_value = (touchpad_msg_t *) evt;
    int touch_num = recv_value->num;
    DeviceNotifyMsg msg = DEVICE_NOTIFY_KEY_UNDEFINED;

    switch (touch_num) {

#if IDF_3_0 == 1
    case TOUCH_PAD_NUM9:
#else
    #if WROAR_BOARD
    case TOUCH_PAD_NUM8:
    #else
    case TOUCH_PAD_NUM4:
    #endif
#endif
    {
        if (recv_value->event == TOUCHPAD_EVENT_TAP) {
            ESP_LOGI(TOUCH_TAG, "touch Switch Mode");
            //TODO
        } else if (recv_value->event == TOUCHPAD_EVENT_LONG_PRESS) {
            //FIXME: does not work now
            ESP_LOGI(TOUCH_TAG, "touch Setting");
            msg = DEVICE_NOTIFY_KEY_WIFI_SET;
        }
    }
    break;
    case TOUCH_PAD_NUM7: {
        if (recv_value->event == TOUCHPAD_EVENT_TAP) {
            ESP_LOGI(TOUCH_TAG, "touch Volume Up Event");
            msg = DEVICE_NOTIFY_KEY_VOL_UP;
        } else if (recv_value->event == TOUCHPAD_EVENT_LONG_PRESS) {
            ESP_LOGI(TOUCH_TAG, "touch Prev Event");
            msg = DEVICE_NOTIFY_KEY_PREV;
        }
    }
    break;
#if IDF_3_0 == 1
    case TOUCH_PAD_NUM8:
#else
    #if WROAR_BOARD
    case TOUCH_PAD_NUM9:
    #else
    case TOUCH_PAD_NUM8:
    #endif
#endif
    {
        if (recv_value->event == TOUCHPAD_EVENT_TAP) {
            msg = DEVICE_NOTIFY_KEY_PLAY;
            ESP_LOGI(TOUCH_TAG, "Play Event");
        }
#if WROAR_BOARD == 0
        else if (recv_value->event == TOUCHPAD_EVENT_LONG_PRESS) {
            //FIXME: does not work now
            ESP_LOGI(TOUCH_TAG, "touch pad event long press rec ");
            msg = DEVICE_NOTIFY_KEY_REC;
        }  else if (recv_value->event == TOUCHPAD_EVENT_RELEASE) {
            ESP_LOGI(TOUCH_TAG, "touch pad long rec RELEASE");
            msg = DEVICE_NOTIFY_KEY_REC_QUIT;
        }
#else
        else {
            ESP_LOGI(TOUCH_TAG, "long press, undefined act");
        }
#endif
    }
    break;
#if WROAR_BOARD
    case TOUCH_PAD_NUM4: {
#else
    case TOUCH_PAD_NUM9: {
#endif
        if (recv_value->event == TOUCHPAD_EVENT_TAP) {
            ESP_LOGI(TOUCH_TAG, "touch Volume Down Event");
            msg = DEVICE_NOTIFY_KEY_VOL_DOWN;
        } else if (recv_value->event == TOUCHPAD_EVENT_LONG_PRESS) {
            ESP_LOGI(TOUCH_TAG, "touch Next Event");
            msg = DEVICE_NOTIFY_KEY_NEXT;
        }
    }
    break;
#if WROAR_BOARD
    case GPIO_REC: {
        if (recv_value->event == TOUCHPAD_EVENT_TAP) {
            ESP_LOGI(TOUCH_TAG, "touch Rec Event");
            msg = DEVICE_NOTIFY_KEY_REC;
        } else if (recv_value->event == TOUCHPAD_EVENT_LONG_PRESS) {
            ESP_LOGI(TOUCH_TAG, "touch Rec playback Event");
            msg = DEVICE_NOTIFY_KEY_REC_QUIT;
        }
    }
    break;
    case GPIO_MODE: {
        if (recv_value->event == TOUCHPAD_EVENT_TAP) {
            ESP_LOGI(TOUCH_TAG, "touch change mode Event");
            msg = DEVICE_NOTIFY_KEY_MODE;
        }
#if (!defined CONFIG_ENABLE_TURINGAIWIFI_SERVICE) && \
    (!defined CONFIG_ENABLE_TURINGWECHAT_SERVICE)
        else if (recv_value->event == TOUCHPAD_EVENT_LONG_PRESS) {
            ESP_LOGI(TOUCH_TAG, "touch to switch App(Wi-Fi or Bt)");
            msg = DEVICE_NOTIFY_KEY_BT_WIFI_SWITCH;
        }
#else
    else if (recv_value->event == TOUCHPAD_EVENT_LONG_PRESS) {
            ESP_LOGI(TOUCH_TAG, "touch to switch aiwifi or wechat)");
            msg = DEVICE_NOTIFY_KEY_AIWIFI_WECHAT_SWITCH;
          }

#endif
    }
    break;
#endif
    default:
        ESP_LOGI(TOUCH_TAG, "Not supported line=%d", __LINE__);
        break;

    }

    if (msg != DEVICE_NOTIFY_KEY_UNDEFINED) {
        manager->Based.notify((struct DeviceController *) manager->Based.controller, DEVICE_NOTIFY_TYPE_TOUCH, &msg, sizeof(DeviceNotifyMsg));
    }
}

static void TouchManagerEvtTask(void *pvParameters)
{
    TouchManager *manager = (TouchManager *) pvParameters;
    touchpad_msg_t recv_value = {0};
    while (1) {
        if (xQueueReceive(xQueueTouch, &recv_value, portMAX_DELAY)) {
            ProcessTouchEvent(manager, &recv_value);
#if EN_STACK_TRACKER
            if(uxTaskGetStackHighWaterMark(TouchManageHandle) > HighWaterLine){
                HighWaterLine = uxTaskGetStackHighWaterMark(TouchManageHandle);
                ESP_LOGI("STACK", "%s %d\n\n\n", __func__, HighWaterLine);
            }
#endif
        }
    }
    vTaskDelete(NULL);
}

static touchpad_msg_t gpioPadMsg;
static char AdcBugFixRec = 0, AdcBugFixMode = 0;
static void GpioRecPushCb(void* arg)
{
    gpioPadMsg.num = GPIO_REC;
    gpioPadMsg.event = TOUCHPAD_EVENT_TAP;
    xQueueSend( xQueueTouch, ( void * ) &gpioPadMsg, ( TickType_t ) 0 );
    AdcBugFixRec = 1;
}

static void GpioRecReleaseCb(void* arg)
{
    gpioPadMsg.num = GPIO_REC;
    gpioPadMsg.event = TOUCHPAD_EVENT_LONG_PRESS;
    if (AdcBugFixRec) {
        AdcBugFixRec = 0;
        xQueueSend( xQueueTouch, ( void * ) &gpioPadMsg, ( TickType_t ) 0 );
    }
}
static void GpioModeTapCb(void* arg)
{
    gpioPadMsg.num = GPIO_MODE;
    gpioPadMsg.event = TOUCHPAD_EVENT_TAP;
    xQueueSend( xQueueTouch, ( void * ) &gpioPadMsg, ( TickType_t ) 0 );
    AdcBugFixMode = 1;
}

static void GpioModePressCb(void* arg)
{
    gpioPadMsg.num = GPIO_MODE;
    gpioPadMsg.event = TOUCHPAD_EVENT_LONG_PRESS;
    if (AdcBugFixMode) {
        AdcBugFixMode = 0;
        xQueueSend( xQueueTouch, ( void * ) &gpioPadMsg, ( TickType_t ) 0 );
    }
}

static void btnCallback(int id, BtnState state)
{
    switch (id) {
    case USER_KEY_SET:
        if (BTN_STATE_CLICK == state) {

        } else if (BTN_STATE_PRESS == state) {
            gpioPadMsg.num = 8;
            gpioPadMsg.event = TOUCHPAD_EVENT_LONG_PRESS;
            xQueueSend( xQueueTouch, ( void* ) &gpioPadMsg, ( TickType_t ) 0 );
        }
        break;
    case USER_KEY_PLAY:
        if (BTN_STATE_CLICK == state) {
            gpioPadMsg.num = 9;
            gpioPadMsg.event = TOUCHPAD_EVENT_TAP;
            xQueueSend( xQueueTouch, ( void* ) &gpioPadMsg, ( TickType_t ) 0 );
        } else if (BTN_STATE_PRESS == state) {

        }
        break;
    case USER_KEY_REC:
        if (BTN_STATE_CLICK == state) {
            gpioPadMsg.num = GPIO_REC;
            gpioPadMsg.event = TOUCHPAD_EVENT_TAP;
            xQueueSend( xQueueTouch, ( void* ) &gpioPadMsg, ( TickType_t ) 0 );
        } else if (BTN_STATE_RELEASE == state) {
            gpioPadMsg.num = GPIO_REC;
            gpioPadMsg.event = TOUCHPAD_EVENT_LONG_PRESS;
            xQueueSend( xQueueTouch, ( void* ) &gpioPadMsg, ( TickType_t ) 0 );
        }
        break;
    case USER_KEY_MODE:
        if (BTN_STATE_CLICK == state) {
            gpioPadMsg.num = GPIO_MODE;
            gpioPadMsg.event = TOUCHPAD_EVENT_TAP;
            xQueueSend( xQueueTouch, ( void* ) &gpioPadMsg, ( TickType_t ) 0 );
        } else if (BTN_STATE_PRESS == state) {
            gpioPadMsg.num = GPIO_MODE;
            gpioPadMsg.event = TOUCHPAD_EVENT_LONG_PRESS;
            xQueueSend( xQueueTouch, ( void* ) &gpioPadMsg, ( TickType_t ) 0 );
        }
        break;
    case USER_KEY_VOL_DOWN:
        if (BTN_STATE_CLICK == state) {
            gpioPadMsg.num = 4;
            gpioPadMsg.event = TOUCHPAD_EVENT_TAP;
            xQueueSend( xQueueTouch, ( void* ) &gpioPadMsg, ( TickType_t ) 0 );
        } else if (BTN_STATE_PRESS == state) {
            gpioPadMsg.num = 4;
            gpioPadMsg.event = TOUCHPAD_EVENT_LONG_PRESS;
            xQueueSend( xQueueTouch, ( void* ) &gpioPadMsg, ( TickType_t ) 0 );
        }
        break;
    case USER_KEY_VOL_UP:
        if (BTN_STATE_CLICK == state) {
            gpioPadMsg.num = 7;
            gpioPadMsg.event = TOUCHPAD_EVENT_TAP;
            xQueueSend( xQueueTouch, ( void* ) &gpioPadMsg, ( TickType_t ) 0 );
        } else if (BTN_STATE_PRESS == state) {
            gpioPadMsg.num = 7;
            gpioPadMsg.event = TOUCHPAD_EVENT_LONG_PRESS;
            xQueueSend( xQueueTouch, ( void* ) &gpioPadMsg, ( TickType_t ) 0 );
        }
        break;
    }
}

static int TouchManagerActive(DeviceManager* self)
{
    TouchManager* manager = (TouchManager*) self;
    xQueueTouch = xQueueCreate(TOUCH_EVT_QUEUE_LEN, sizeof(touchpad_msg_t));
    configASSERT(xQueueTouch);
    //touchpad
    touchpad_create(TOUCH_PAD_NUM4, TOUCHPAD_THRESHOLD, TOUCHPAD_FILTER_VALUE, TOUCHPAD_SINGLE_TRIGGER, 2, &xQueueTouch, 10);
    touchpad_create(TOUCH_PAD_NUM7, TOUCHPAD_THRESHOLD, TOUCHPAD_FILTER_VALUE, TOUCHPAD_SINGLE_TRIGGER, 2, &xQueueTouch, 10);
    touchpad_create(TOUCH_PAD_NUM8, TOUCHPAD_THRESHOLD, TOUCHPAD_FILTER_VALUE, TOUCHPAD_SINGLE_TRIGGER, 2, &xQueueTouch, 10);
    touchpad_create(TOUCH_PAD_NUM9, TOUCHPAD_THRESHOLD, TOUCHPAD_FILTER_VALUE, TOUCHPAD_SINGLE_TRIGGER, 2, &xQueueTouch, 10);
    void touchpad_init();
    touchpad_init();
    //button
    button_handle_t btnRec = button_dev_init(GPIO_REC, 0, BUTTON_ACTIVE_LEVEL);
    button_dev_add_tap_cb(BUTTON_PUSH_CB, GpioRecPushCb, "PUSH", 50 / portTICK_PERIOD_MS, btnRec);
    button_dev_add_tap_cb(BUTTON_RELEASE_CB, GpioRecReleaseCb, "RELEASE", 50 / portTICK_PERIOD_MS, btnRec);

    button_handle_t btnMode = button_dev_init(GPIO_MODE, 1, BUTTON_ACTIVE_LEVEL);
    button_dev_add_tap_cb(BUTTON_TAP_CB, GpioModeTapCb, "TAP", 50 / portTICK_PERIOD_MS, btnMode);
    button_dev_add_press_cb(0, GpioModePressCb, NULL, 2000 / portTICK_PERIOD_MS, btnMode);

    ESP_LOGI(TOUCH_TAG, "manager active, freemem %d, %p", esp_get_free_heap_size(), xQueueTouch);
    // adc_btn_init(btnCallback);
    if (xTaskCreatePinnedToCore(TouchManagerEvtTask,
                                "TouchManagerEvtTask",
                                TOUCH_MANAGER_TASK_STACK_SIZE,
                                manager,
                                TOUCH_MANAGER_TASK_PRIORITY,
                                &TouchManageHandle, xPortGetCoreID()) != pdPASS) {
        ESP_LOGE(TOUCH_TAG, "Error create TouchManagerTask");
        return -1;
    }
    return 0;
}

static void TouchManagerDeactive(DeviceManager *self)
{
    //TODO
    ESP_LOGI(TOUCH_TAG, "TouchManagerStop\r\n");
    vQueueDelete(xQueueTouch);
    xQueueTouch = NULL;
}

TouchManager *TouchManagerCreate(struct DeviceController *controller)
{
    if (!controller)
        return NULL;
    ESP_LOGI(TOUCH_TAG, "TouchManagerCreate\r\n");
    TouchManager *touch = (TouchManager *) calloc(1, sizeof(TouchManager));
    ESP_ERROR_CHECK(!touch);
    InitManager((DeviceManager *) touch, controller);
    touch->Based.active = TouchManagerActive;
    touch->Based.deactive = TouchManagerDeactive;
    touch->Based.notify = TouchpadEvtNotify;  //TODO
    return touch;
}
