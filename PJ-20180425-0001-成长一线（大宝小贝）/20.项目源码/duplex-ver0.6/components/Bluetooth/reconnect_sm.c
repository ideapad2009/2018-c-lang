#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "reconnect_sm.h"
#include "bt_app_common.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

typedef enum {
    A2D_REC_STATE_IDLE,
    A2D_REC_STATE_OPEN,
    A2D_REC_STATE_MAX
} a2d_rec_state_t;

typedef struct {
    a2d_rec_state_t state;
    uint32_t rec_cnt; /* reconnection count */
    a2d_connect_func connect;
    TimerHandle_t tmr;
} a2d_rec_cb_t;

#define BT_REC_TAG           "BT_REC"
#define MAX_REC_INTV          (600)

const uint8_t m_rec_tbl[] = {5, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20};
const uint32_t m_rec_tbl_len = (sizeof(m_rec_tbl) / sizeof(m_rec_tbl[0]));

#define A2D_REC_TMR_WAIT_TICKS  (100 / portTICK_PERIOD_MS)


static a2d_rec_cb_t m_a2d_rec_cb;

extern void a2d_rec_alarm_to(void *p);

bool a2d_rec_sm_init(a2d_connect_func connect)
{
    int tmr_id = 0;
    memset(&m_a2d_rec_cb, 0, sizeof(a2d_rec_cb_t));

    m_a2d_rec_cb.tmr = xTimerCreate("recSmTmr", (500 / portTICK_RATE_MS),
                                    pdFALSE, (void *)tmr_id, a2d_rec_alarm_to);
    if (m_a2d_rec_cb.tmr == 0 ) {
        return false;
    }
    if (connect) {
        m_a2d_rec_cb.connect = connect;
    }
    ESP_LOGD(BT_REC_TAG, "%s ok\n", __func__);
    return true;
}

void a2d_rec_sm_deinit(void)
{
    xTimerDelete(m_a2d_rec_cb.tmr, A2D_REC_TMR_WAIT_TICKS);
    m_a2d_rec_cb.tmr = 0;
    memset(&m_a2d_rec_cb, 0, sizeof(a2d_rec_cb_t));
}

static uint32_t a2d_rec_wait_intv(uint32_t rec_cnt)
{
    if (rec_cnt >= m_rec_tbl_len) {
        return 1000ul * MAX_REC_INTV;
    }
    return 1000ul * m_rec_tbl[rec_cnt];
}

static void a2d_rec_sm_idle_handler(uint32_t event, void *data)
{
    a2d_rec_cb_t *p_cb = &m_a2d_rec_cb;
    ESP_LOGD(BT_REC_TAG, "Idle state, evt %u\n", event);
    assert(p_cb->tmr != 0);
    if (event == A2D_REC_EVT_DISCONNECTED) {
        p_cb->rec_cnt = 0;
        uint32_t timeout_ms = a2d_rec_wait_intv(p_cb->rec_cnt);
        xTimerChangePeriod(p_cb->tmr, timeout_ms / portTICK_PERIOD_MS, A2D_REC_TMR_WAIT_TICKS);
        xTimerStart(p_cb->tmr, A2D_REC_TMR_WAIT_TICKS);
        p_cb->state = A2D_REC_STATE_OPEN;
    }
}

static void a2d_rec_sm_open_handler(uint32_t event, void *data)
{
    a2d_rec_cb_t *p_cb = &m_a2d_rec_cb;
    ESP_LOGD(BT_REC_TAG, "Open state, evt %u\n", event);
    assert(p_cb->tmr != 0);
    if (event == A2D_REC_EVT_CONNECTED) {
        xTimerStop(p_cb->tmr, A2D_REC_TMR_WAIT_TICKS);
        p_cb->rec_cnt = 0;
        p_cb->state = A2D_REC_STATE_IDLE;
    } else if (event == A2D_REC_EVT_ALARM_TO) {
        if (p_cb->connect != NULL) {
            p_cb->connect();
        }
        p_cb->rec_cnt ++;
        uint32_t timeout_ms = a2d_rec_wait_intv(p_cb->rec_cnt);
        xTimerChangePeriod(p_cb->tmr, timeout_ms / portTICK_PERIOD_MS, A2D_REC_TMR_WAIT_TICKS);
        xTimerStart(p_cb->tmr, A2D_REC_TMR_WAIT_TICKS);
    }
}

void a2d_rec_sm_handler(uint32_t event, void *data)
{
    a2d_rec_cb_t *p_cb = &m_a2d_rec_cb;
    ESP_LOGI(BT_REC_TAG, "SM hdlr: st %u, evt %u\n", p_cb->state, event);
    switch (p_cb->state) {
    case A2D_REC_STATE_IDLE:
        a2d_rec_sm_idle_handler(event, data);
        break;
    case A2D_REC_STATE_OPEN:
        a2d_rec_sm_open_handler(event, data);
        break;
    default:
        break;
    }
}
