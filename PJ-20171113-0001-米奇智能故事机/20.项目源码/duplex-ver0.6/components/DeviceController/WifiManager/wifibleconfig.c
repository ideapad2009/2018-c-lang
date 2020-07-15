
// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "sdkconfig.h"

#ifdef CONFIG_BLUEDROID_ENABLED
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_event.h"

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "bt.h"

#include "esp_blufi_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

#include "key_control.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "../DeviceController/DeviceCommon.h"
#include "MediaControl.h"
#include "AudioCodec.h"
#include "toneuri.h"
#include "AudioDef.h"
#include "EspAudioAlloc.h"
#include "wifibleconfig.h"
#include "userconfig.h"
#include "esp_heap_alloc_caps.h"

#include "mbedtls/aes.h"
#include "mbedtls/dhm.h"
#include "mbedtls/md5.h"
#include "rom/crc.h"

#define WIFI_BLE_TAG "WIFI_BLE_CONFIG"

/*
   The SEC_TYPE_xxx is for self-defined packet data type in the procedure of "BLUFI negotiate key"
   If user use other negotiation procedure to exchange(or generate) key, should redefine the type by yourself.
 */
#define SEC_TYPE_DH_PARAM_LEN   0x00
#define SEC_TYPE_DH_PARAM_DATA  0x01
#define SEC_TYPE_DH_P           0x02
#define SEC_TYPE_DH_G           0x03
#define SEC_TYPE_DH_PUBLIC      0x04


struct blufi_security {
#define DH_SELF_PUB_KEY_LEN     128
#define DH_SELF_PUB_KEY_BIT_LEN (DH_SELF_PUB_KEY_LEN * 8)
    uint8_t  self_public_key[DH_SELF_PUB_KEY_LEN];
#define SHARE_KEY_LEN           128
#define SHARE_KEY_BIT_LEN       (SHARE_KEY_LEN * 8)
    uint8_t  share_key[SHARE_KEY_LEN];
    size_t   share_len;
#define PSK_LEN                 16
    uint8_t  psk[PSK_LEN];
    uint8_t  *dh_param;
    int      dh_param_len;
    uint8_t  iv[16];
    mbedtls_dhm_context dhm;
    mbedtls_aes_context aes;
};
static struct blufi_security *blufi_sec;

#define BLE_DONE_EVT        BIT0
#define BLE_STOP_REQ_EVT    BIT1


EventGroupHandle_t g_ble_event_group = NULL;
static xSemaphoreHandle g_ble_mux = NULL;

static void audio_ble_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param);

#define BLUFI_DEVICE_NAME            "BLUFI_DEVICE"
static uint8_t audio_ble_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
};

//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
static esp_ble_adv_data_t audio_ble_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x100,
    .max_interval = 0x100,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 16,
    .p_service_uuid = audio_ble_service_uuid128,
    .flag = 0x6,
};

static esp_ble_adv_params_t audio_ble_adv_params = {
    .adv_int_min        = 0x100,
    .adv_int_max        = 0x100,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

#define WIFI_LIST_NUM   10

static wifi_config_t sta_config;
static wifi_config_t ap_config;

/* store the station info for send back to phone */
uint8_t gl_sta_bssid[6];
uint8_t gl_sta_ssid[32];
int gl_sta_ssid_len;

/* connect infor*/
static uint8_t server_if;
static uint16_t conn_id;

static int myrand( void *rng_state, unsigned char *output, size_t len )
{
    size_t i;

    for( i = 0; i < len; ++i )
        output[i] = esp_random();

    return( 0 );
}

void blufi_dh_negotiate_data_handler(uint8_t *data, int len, uint8_t **output_data, int *output_len, bool *need_free)
{
    int ret;
    uint8_t type = data[0];

    if (blufi_sec == NULL) {
        ESP_LOGE(WIFI_BLE_TAG ,"BLUFI Security is not initialized");
        return;
    }

    switch (type) {
    case SEC_TYPE_DH_PARAM_LEN:
        blufi_sec->dh_param_len = ((data[1]<<8)|data[2]);
        if (blufi_sec->dh_param) {
            free(blufi_sec->dh_param);
        }
        blufi_sec->dh_param = (uint8_t *)malloc(blufi_sec->dh_param_len);
        if (blufi_sec->dh_param == NULL) {
            return;
        }
        break;
    case SEC_TYPE_DH_PARAM_DATA:

        memcpy(blufi_sec->dh_param, &data[1], blufi_sec->dh_param_len);

        ret = mbedtls_dhm_read_params(&blufi_sec->dhm, &blufi_sec->dh_param, &blufi_sec->dh_param[blufi_sec->dh_param_len]);
        if (ret) {
            ESP_LOGE(WIFI_BLE_TAG ,"%s read param failed %d\n", __func__, ret);
            return;
        }

        ret = mbedtls_dhm_make_public(&blufi_sec->dhm, (int) mbedtls_mpi_size( &blufi_sec->dhm.P ), blufi_sec->self_public_key, blufi_sec->dhm.len, myrand, NULL);
        if (ret) {
            ESP_LOGE(WIFI_BLE_TAG ,"%s make public failed %d\n", __func__, ret);
            return;
        }

        mbedtls_dhm_calc_secret( &blufi_sec->dhm,
                blufi_sec->share_key,
                SHARE_KEY_BIT_LEN,
                &blufi_sec->share_len,
                NULL, NULL);

        mbedtls_md5(blufi_sec->share_key, blufi_sec->share_len, blufi_sec->psk);

        mbedtls_aes_setkey_enc(&blufi_sec->aes, blufi_sec->psk, 128);

        /* alloc output data */
        *output_data = &blufi_sec->self_public_key[0];
        *output_len = blufi_sec->dhm.len;
        *need_free = false;
        break;
    case SEC_TYPE_DH_P:
        break;
    case SEC_TYPE_DH_G:
        break;
    case SEC_TYPE_DH_PUBLIC:
        break;
    }
}

int blufi_aes_encrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len)
{
    int ret;
    size_t iv_offset = 0;
    uint8_t iv0[16];

    memcpy(iv0, blufi_sec->iv, sizeof(blufi_sec->iv));
    iv0[0] = iv8;   /* set iv8 as the iv0[0] */

    ret = mbedtls_aes_crypt_cfb128(&blufi_sec->aes, MBEDTLS_AES_ENCRYPT, crypt_len, &iv_offset, iv0, crypt_data, crypt_data);
    if (ret) {
        return -1;
    }

    return crypt_len;
}

int blufi_aes_decrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len)
{
    int ret;
    size_t iv_offset = 0;
    uint8_t iv0[16];

    memcpy(iv0, blufi_sec->iv, sizeof(blufi_sec->iv));
    iv0[0] = iv8;   /* set iv8 as the iv0[0] */

    ret = mbedtls_aes_crypt_cfb128(&blufi_sec->aes, MBEDTLS_AES_DECRYPT, crypt_len, &iv_offset, iv0, crypt_data, crypt_data);
    if (ret) {
        return -1;
    }

    return crypt_len;
}

uint16_t blufi_crc_checksum(uint8_t iv8, uint8_t *data, int len)
{
    /* This iv8 ignore, not used */
    return crc16_be(0, data, len);
}

esp_err_t blufi_security_init(void)
{
    blufi_sec = (struct blufi_security *)malloc(sizeof(struct blufi_security));
    if (blufi_sec == NULL) {
        return ESP_FAIL;
    }

    memset(blufi_sec, 0x0, sizeof(struct blufi_security));

    mbedtls_dhm_init(&blufi_sec->dhm);
    mbedtls_aes_init(&blufi_sec->aes);

    memset(blufi_sec->iv, 0x0, 16);
    return 0;
}

void blufi_security_deinit(void)
{
    mbedtls_dhm_free(&blufi_sec->dhm);
    mbedtls_aes_free(&blufi_sec->aes);

    memset(blufi_sec, 0x0, sizeof(struct blufi_security));

    free(blufi_sec);
    blufi_sec =  NULL;
}

static esp_blufi_callbacks_t audio_ble_callbacks = {
    .event_cb = audio_ble_event_callback,
    .negotiate_data_handler = blufi_dh_negotiate_data_handler,
    .encrypt_func = blufi_aes_encrypt,
    .decrypt_func = blufi_aes_decrypt,
    .checksum_func = blufi_crc_checksum,
};

static void audio_ble_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    /* actually, should post to blufi_task handle the procedure,
     * now, as a audio_ble, we do it more simply */
    esp_err_t ret ;
    switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
        ESP_LOGI(WIFI_BLE_TAG ,"BLUFI init finish\n");

        esp_ble_gap_set_device_name(BLUFI_DEVICE_NAME);
        esp_ble_gap_config_adv_data(&audio_ble_adv_data);
        break;
    case ESP_BLUFI_EVENT_DEINIT_FINISH:
        ESP_LOGI(WIFI_BLE_TAG ,"BLUFI deinit finish\n");
        break;
    case ESP_BLUFI_EVENT_BLE_CONNECT:
        ESP_LOGI(WIFI_BLE_TAG ,"BLUFI ble connect\n");
        server_if=param->connect.server_if;
        conn_id=param->connect.conn_id;
        esp_ble_gap_stop_advertising();
        blufi_security_deinit();
        blufi_security_init();
        break;
    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
        ESP_LOGI(WIFI_BLE_TAG ,"BLUFI ble disconnect\n");
        esp_ble_gap_start_advertising(&audio_ble_adv_params);
        break;
    case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
        ESP_LOGI(WIFI_BLE_TAG ,"BLUFI Set WIFI opmode %d\n", param->wifi_mode.op_mode);
        ESP_ERROR_CHECK( esp_wifi_set_mode(param->wifi_mode.op_mode) );
        break;
    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
        ESP_LOGI(WIFI_BLE_TAG ,"BLUFI requset wifi connect to AP\n");
        xEventGroupSetBits(g_ble_event_group, BLE_CONFIGING);
        if(ESP_OK != esp_wifi_connect()){
            xEventGroupSetBits(g_ble_event_group, BLE_STOP_REQ_EVT);
        }
        break;
    case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
        ESP_LOGI(WIFI_BLE_TAG ,"BLUFI requset wifi disconnect from AP\n");
        esp_wifi_disconnect();
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_STATUS: {
        wifi_mode_t mode;
        esp_blufi_extra_info_t info;

        esp_wifi_get_mode(&mode);

        memset(&info, 0, sizeof(esp_blufi_extra_info_t));
        memcpy(info.sta_bssid, gl_sta_bssid, 6);
        info.sta_bssid_set = true;
        info.sta_ssid = gl_sta_ssid;
        info.sta_ssid_len = gl_sta_ssid_len;
        esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, 0, &info);
        xEventGroupSetBits(g_ble_event_group, BLE_DONE_EVT);
        ESP_LOGI(WIFI_BLE_TAG ,"BLUFI get wifi status from AP\n");

        break;
    }
    case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
        ESP_LOGI(WIFI_BLE_TAG ,"blufi close a gatt connection");
        esp_blufi_close(server_if,conn_id);
        break;
    case ESP_BLUFI_EVENT_DEAUTHENTICATE_STA:
        /* TODO */
        break;
    case ESP_BLUFI_EVENT_RECV_STA_BSSID:
        memcpy(sta_config.sta.bssid, param->sta_bssid.bssid, 6);
        sta_config.sta.bssid_set = 1;
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        ESP_LOGI(WIFI_BLE_TAG ,"Recv STA BSSID %s\n", sta_config.sta.bssid);
        break;
    case ESP_BLUFI_EVENT_RECV_STA_SSID:
        strncpy((char *)sta_config.sta.ssid, (char *)param->sta_ssid.ssid, param->sta_ssid.ssid_len);
        sta_config.sta.ssid[param->sta_ssid.ssid_len] = '\0';
        ret = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        ESP_LOGI(WIFI_BLE_TAG ,"Recv STA SSID ret %d %s\n", ret,sta_config.sta.ssid);
        break;
    case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
        strncpy((char *)sta_config.sta.password, (char *)param->sta_passwd.passwd, param->sta_passwd.passwd_len);
        sta_config.sta.password[param->sta_passwd.passwd_len] = '\0';
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        ESP_LOGI(WIFI_BLE_TAG ,"Recv STA PASSWORD %s\n", sta_config.sta.password);
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_SSID:
        strncpy((char *)ap_config.ap.ssid, (char *)param->softap_ssid.ssid, param->softap_ssid.ssid_len);
        ap_config.ap.ssid_len = param->softap_ssid.ssid_len;
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        ESP_LOGI(WIFI_BLE_TAG ,"Recv SOFTAP SSID %s, ssid len %d\n", ap_config.ap.ssid, ap_config.ap.ssid_len);
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_PASSWD:
        strncpy((char *)ap_config.ap.password, (char *)param->softap_passwd.passwd, param->softap_passwd.passwd_len);
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        ESP_LOGI(WIFI_BLE_TAG ,"Recv SOFTAP PASSWORD %s\n", ap_config.ap.password);
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM:
        if (param->softap_max_conn_num.max_conn_num > 4) {
            return;
        }
        ap_config.ap.max_connection = param->softap_max_conn_num.max_conn_num;
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        ESP_LOGI(WIFI_BLE_TAG ,"Recv SOFTAP MAX CONN NUM %d\n", ap_config.ap.max_connection);
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_AUTH_MODE:
        if (param->softap_auth_mode.auth_mode >= WIFI_AUTH_MAX) {
            return;
        }
        ap_config.ap.authmode = param->softap_auth_mode.auth_mode;
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        ESP_LOGI(WIFI_BLE_TAG ,"Recv SOFTAP AUTH MODE %d\n", ap_config.ap.authmode);
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_CHANNEL:
        if (param->softap_channel.channel > 13) {
            return;
        }
        ap_config.ap.channel = param->softap_channel.channel;
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        ESP_LOGI(WIFI_BLE_TAG ,"Recv SOFTAP CHANNEL %d\n", ap_config.ap.channel);
        break;
    case ESP_BLUFI_EVENT_RECV_USERNAME:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_CA_CERT:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_CLIENT_CERT:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_SERVER_CERT:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_CLIENT_PRIV_KEY:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_SERVER_PRIV_KEY:
        /* Not handle currently */
        break;
    default:
        break;
    }
}

static void audio_ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&audio_ble_adv_params);
        break;
    default:
        break;
    }
}

esp_err_t BleconfigSetup(void)
{
    if (g_ble_event_group == NULL) {
        g_ble_event_group = xEventGroupCreate();
        if(g_ble_event_group == NULL){
           ESP_LOGE(WIFI_BLE_TAG, "g_ble_event_group creat failed!");
           return ESP_FAIL;
        }
    }
    if (g_ble_mux == NULL) {
        g_ble_mux = xSemaphoreCreateMutex();
        if (g_ble_mux == NULL) {
            ESP_LOGE(WIFI_BLE_TAG, "g_ble_mux creat failed!");
            vEventGroupDelete(g_ble_event_group);
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t BleConfigStart(uint32_t ticks_to_wait)
{
    ESP_LOGI(WIFI_BLE_TAG, "BlufiConfigServiceCreate\r\n");
    esp_err_t ret;

    portBASE_TYPE res = xSemaphoreTake(g_ble_mux, ticks_to_wait);
    if (res != pdPASS) {
        return ESP_ERR_TIMEOUT;
    }

    xEventGroupClearBits(g_ble_event_group, BLE_STOP_REQ_EVT);
    ESP_LOGI(WIFI_BLE_TAG, "BluetoothControlCreate\r\n");
    esp_bt_controller_status_t status = esp_bt_controller_get_status();
    if(status == ESP_BT_CONTROLLER_STATUS_IDLE) {
        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
            ESP_LOGE(WIFI_BLE_TAG, "%s initialize controller failed\n", __func__);
            return ESP_FAIL;
        }
        if (esp_bt_controller_enable(ESP_BT_MODE_BLE) != ESP_OK) {
            ESP_LOGE(WIFI_BLE_TAG, "%s enable controller failed\n", __func__);
            return ESP_FAIL;
        }
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(WIFI_BLE_TAG, "%s init bluedroid failed\n", __func__);
        return ret;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(WIFI_BLE_TAG, "%s init bluedroid failed\n", __func__);
        return ret;
    }
    ESP_LOGI(WIFI_BLE_TAG , "BD ADDR: "ESP_BD_ADDR_STR"\n", ESP_BD_ADDR_HEX(esp_bt_dev_get_address()));

    ESP_LOGI(WIFI_BLE_TAG , "BLUFI VERSION %04x\n", esp_blufi_get_version());

    blufi_security_init();
    esp_ble_gap_register_callback(audio_ble_gap_event_handler);

    esp_blufi_register_callbacks(&audio_ble_callbacks);
    esp_blufi_profile_init();
    EventBits_t uxBits;
    uxBits = xEventGroupWaitBits(g_ble_event_group, BLE_DONE_EVT | BLE_STOP_REQ_EVT, true, false, ticks_to_wait);
    // WiFi connected event
    if (uxBits & BLE_DONE_EVT) {
        ESP_LOGD(WIFI_BLE_TAG, "WiFi connected");
        ret = ESP_OK;
    }
    // WiFi stop connecting event
    else if (uxBits & BLE_STOP_REQ_EVT) {
        ESP_LOGD(WIFI_BLE_TAG, "Bleconfig stop.");
        ret = ESP_FAIL;
    }
    // WiFi connect timeout
    else {
        esp_wifi_stop();
        ESP_LOGE(WIFI_BLE_TAG, "WiFi connect fail");
        ret = ESP_ERR_TIMEOUT;
    }
    xSemaphoreGive(g_ble_mux);
    return ret;
}
#endif
