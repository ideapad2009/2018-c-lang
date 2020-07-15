#ifndef __A2D_RECONNECT_SM_H__
#define __A2D_RECONNECT_SM_H__

#include <stdbool.h>
#include <stdint.h>

#define A2D_REC_EVT_DISCONNECTED       (0)
#define A2D_REC_EVT_CONNECTED          (1)
#define A2D_REC_EVT_ALARM_TO           (2)
#define A2D_REC_EVT_MAX                (3)

typedef void (* a2d_connect_func)(void);

bool a2d_rec_sm_init(a2d_connect_func connect);

void a2d_rec_sm_deinit(void);
void a2d_rec_sm_handler(uint32_t event, void *data);
#endif /* __A2D_RECONNECT_SM_H__ */
