// Copyright 2010-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "wifismartconfig.h"
//#include "webserver.h"  //FIXME: is this needed??

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_types.h"
#include "esp_log.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"

#define SC_DONE_EVT        BIT0
#define SC_STOP_REQ_EVT    BIT1

#define IOT_CHECK(tag, a, ret)  if(!(a)) {                                             \
        ESP_LOGE(tag,"%s:%d (%s)", __FILE__, __LINE__, __FUNCTION__);      \
        return (ret);                                                                   \
        }

#define ERR_ASSERT(tag, param)  IOT_CHECK(tag, (param) == ESP_OK, ESP_FAIL)

#define WIFI_SC_TAG "WIFI_SMART_CONFIG"

static xSemaphoreHandle g_sc_mux = NULL;
EventGroupHandle_t g_sc_event_group = NULL;

/******************************************************************************
 * FunctionName : smartconfig_done
 * Description  : callback function which be called during the samrtconfig process
 * Parameters   : status -- the samrtconfig status
 *                pdata --
 * Returns      : none
*******************************************************************************/
static void smartconfig_done(smartconfig_status_t status, void *pdata)
{
    wifi_config_t sta_conf;

    switch (status) {
    case SC_STATUS_WAIT:
        ESP_LOGI(WIFI_SC_TAG, "SC_STATUS_WAIT");
        break;
    case SC_STATUS_FIND_CHANNEL:
        ESP_LOGI(WIFI_SC_TAG, "SC_STATUS_FIND_CHANNEL");
        break;
    case SC_STATUS_GETTING_SSID_PSWD:
        ESP_LOGI(WIFI_SC_TAG, "SC_STATUS_GETTING_SSID_PSWD");
        smartconfig_type_t *type = pdata;
        if (*type == SC_TYPE_ESPTOUCH) {
            ESP_LOGD(WIFI_SC_TAG, "SC_TYPE:SC_TYPE_ESPTOUCH");
        } else {
            ESP_LOGD(WIFI_SC_TAG, "SC_TYPE:SC_TYPE_AIRKISS");
        }
        break;
    case SC_STATUS_LINK:
        ESP_LOGI(WIFI_SC_TAG, "SC_STATUS_LINK");

        //EspAudio_SetupStream("40_start_network.mp3",InputSrcType_LocalFile);
        esp_wifi_disconnect();
        memset(&sta_conf, 0x00, sizeof(sta_conf));
        memcpy(&(sta_conf.sta), pdata, sizeof(wifi_sta_config_t));
        ESP_LOGI(WIFI_SC_TAG, "<link>ssid:%s", sta_conf.sta.ssid);
        ESP_LOGI(WIFI_SC_TAG, "<link>pass:%s", sta_conf.sta.password);

        if (esp_wifi_set_config(WIFI_IF_STA, &sta_conf)) {
            ESP_LOGE(WIFI_SC_TAG, "[%s] set_config fail", __func__);
        }
        if (esp_wifi_connect()) {
            ESP_LOGE(WIFI_SC_TAG, "[%s] wifi_connect fail", __func__);
        }
        break;
    case SC_STATUS_LINK_OVER:
        ESP_LOGI(WIFI_SC_TAG, "SC_STATUS_LINK_OVER");
        if (pdata != NULL) {
            uint8_t phone_ip[4] = {0};
            memcpy(phone_ip, (const void *)pdata, 4);
            ESP_LOGI(WIFI_SC_TAG, "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
        }
        //esp_smartconfig_stop();
        xEventGroupSetBits(g_sc_event_group, SC_DONE_EVT);
        break;
    }

}

esp_err_t SmartconfigSetup(smartconfig_type_t sc_type, bool fast_mode_en)
{
    if (g_sc_event_group == NULL) {
        g_sc_event_group = xEventGroupCreate();
        if(g_sc_event_group == NULL){
           ESP_LOGE(WIFI_SC_TAG, "g_sc_event_group creat failed!");
           return ESP_FAIL;
        }
        ESP_ERROR_CHECK(esp_smartconfig_set_type(sc_type));
        ERR_ASSERT(WIFI_SC_TAG, esp_smartconfig_fast_mode(fast_mode_en));
    }
    if (g_sc_mux == NULL) {
        g_sc_mux = xSemaphoreCreateMutex();
        if (g_sc_mux == NULL) {
            ESP_LOGE(WIFI_SC_TAG, "g_sc_mux creat failed!");
            vEventGroupDelete(g_sc_event_group);
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t WifiSmartConnect(uint32_t ticks_to_wait)
{
    static bool inSmartconfig = false;
    if (inSmartconfig) {
        return ESP_ERR_INVALID_STATE;
    }
    ESP_LOGI(WIFI_SC_TAG, "***** smartconfig_start is starting *****");
    portBASE_TYPE res = xSemaphoreTake(g_sc_mux, ticks_to_wait);
    if (res != pdPASS) {
        return ESP_ERR_TIMEOUT;
    }
    xEventGroupClearBits(g_sc_event_group, SC_STOP_REQ_EVT);
    esp_wifi_start();
    ESP_ERROR_CHECK(esp_smartconfig_start(smartconfig_done, 1));
    inSmartconfig = true;
    // Wait event bits
    EventBits_t uxBits;
    uxBits = xEventGroupWaitBits(g_sc_event_group, SC_DONE_EVT | SC_STOP_REQ_EVT, true, false, ticks_to_wait);
    esp_err_t ret;
    // WiFi connected event
    if (uxBits & SC_DONE_EVT) {
        ESP_LOGD(WIFI_SC_TAG, "WiFi connected");
        ret = ESP_OK;
    }
    // WiFi stop connecting event
    else if (uxBits & SC_STOP_REQ_EVT) {
        ESP_LOGD(WIFI_SC_TAG, "Smartconfig stop.");
        ret = ESP_FAIL;
    }
    // WiFi connect timeout
    else {
        esp_wifi_stop();
        ESP_LOGE(WIFI_SC_TAG, "WiFi connect fail");
        ret = ESP_ERR_TIMEOUT;
    }
    esp_smartconfig_stop();
    inSmartconfig = false;
    xSemaphoreGive(g_sc_mux);
    xEventGroupClearBits(g_sc_event_group, SC_PAIRING);
    return ret;
}
