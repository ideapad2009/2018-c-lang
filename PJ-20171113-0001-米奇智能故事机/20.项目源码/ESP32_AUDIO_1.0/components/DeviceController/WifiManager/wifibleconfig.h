#ifndef _WIFI_BLECONFIG_H_
#define _WIFI_BLECONFIG_H_
#include "esp_err.h"

#ifdef CONFIG_BLUEDROID_ENABLED

#include "esp_blufi_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#endif

#define BLE_CONFIGING       BIT2

esp_err_t BleconfigSetup(void);
esp_err_t BleConfigStart(uint32_t ticks_to_wait);

#endif
