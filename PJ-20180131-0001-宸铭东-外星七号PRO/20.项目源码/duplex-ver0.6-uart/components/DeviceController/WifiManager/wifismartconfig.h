#ifndef _WIFI_SMARTCONFIG_H_
#define _WIFI_SMARTCONFIG_H_
#include "esp_err.h"
#include "esp_smartconfig.h"

#define SC_PAIRING         BIT2

esp_err_t SmartconfigSetup(smartconfig_type_t sc_type, bool fast_mode_en);
esp_err_t WifiSmartConnect(uint32_t ticks_to_wait);
void set_event_group_stop_bits();//设置g_sc_event_group的数据为SC_STOP_REQ_EVT

#endif
