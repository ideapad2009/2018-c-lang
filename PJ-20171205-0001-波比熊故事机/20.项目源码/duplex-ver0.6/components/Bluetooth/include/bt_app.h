#ifndef __BT_APP_H__
#define __BT_APP_H__

#include "bt_app_common.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

typedef enum {
    BT_APP_EVT_STACK_ON = 0xa0,
    BT_APP_EVT_MAX
} bt_app_evt_t;

typedef struct {
    int key_code;
    int key_state;
} esp_avrc_key_state_t;

typedef union {
    esp_a2d_cb_param_t a2d;
    esp_avrc_ct_cb_param_t rc;
    esp_avrc_key_state_t key;
} bt_app_evt_arg;

void bt_app_handle_rc_evt(uint16_t event, void *p_param);

#endif /* __BT_APP_H__ */
