#ifndef __BT_APP_COMMON_H__
#define __BT_APP_COMMON_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define BT_APP_SIG_CONTEXT_SWITCH         (0x90)
#define BT_APP_MAX_EXT_QUE_NUM            (2)

typedef void (* bt_app_ext_que_hdl_t) (void *param);

typedef struct {
    uint16_t xque_len;
    uint16_t xque_item_sz;
    xQueueHandle xque;
    bt_app_ext_que_hdl_t xque_hdl;
} bt_app_xque_item_t;

typedef void (* bt_app_cb_t) (uint16_t event, void *param);
/* context switch message */
typedef struct {
    uint16_t             sig;      /* signal to bt_app_task */
    uint16_t             event;    /* message event id */
    bt_app_cb_t          cb;       /* context switch callback */
    void                 *param;   /* parameter area needs to be last */
} bt_app_msg_t;

typedef void (* bt_app_copy_cb_t) (bt_app_msg_t *msg, void *p_dest, void *p_src);

bool bt_app_transfer_context (bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback);

void bt_app_task_start_up(const bt_app_xque_item_t *param, uint32_t xque_num);

void bt_app_task_shut_down(void);
#endif /* __BT_APP_COMMON_H__ */
