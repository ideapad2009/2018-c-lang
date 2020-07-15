#ifndef __KEY_CONTROL_H__
#define __KEY_CONTROL_H__

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "userconfig.h"
typedef enum {
    KEY_ACT_STATE_IDLE,
    KEY_ACT_STATE_PRESS,
    KEY_ACT_STATE_RELEASE
} key_act_state_t;

typedef struct {
    key_act_state_t state;
    uint32_t key_code;
    TimerHandle_t key_tmr;
    uint8_t tl;
} key_act_cb_t;

typedef struct {
    uint32_t evt;
    uint32_t key_code;
    uint8_t key_state;
    uint8_t tl;
} key_act_param_t;

/// AVRC callback events
#if IDF_3_0
#define ESP_AVRC_CT_MAX_EVT (ESP_AVRC_CT_REMOTE_FEATURES_EVT)
#endif // IDF_3_0

#define ESP_AVRC_CT_KEY_STATE_CHG_EVT (ESP_AVRC_CT_MAX_EVT + 1) // key-press action is triggered
#define ESP_AVRC_CT_PT_RSP_TO_EVT (ESP_AVRC_CT_MAX_EVT + 2) // passthrough-command response time-out

void key_control_task_handler(void *arg);

bool key_act_sm_init(void);
void key_act_sm_deinit(void);
void key_act_state_machine(key_act_param_t *param);

#endif /* __KEY_CONTROL_H__ */
