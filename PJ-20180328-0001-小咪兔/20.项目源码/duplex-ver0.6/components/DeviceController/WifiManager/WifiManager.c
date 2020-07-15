#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_err.h"
#include "esp_system.h"
#include "sdkconfig.h"

#include "WifiManager.h"
#include "DeviceCommon.h"
#include "NotifyHelper.h"
#include "wifismartconfig.h"
#include "wifibleconfig.h"
#include "userconfig.h"
#include "toneuri.h"
#include "EspAudio.h"
#include "flash_config_manage.h"
#include "player_middleware.h"
#ifndef CONFIG_CLASSIC_BT_ENABLED

#define WIFICONFIG_TAG                              "WIFI_CONFIG"
#define WIFI_EVT_QUEUE_LEN                          4
#define WIFI_SMARTCONFIG_TASK_PRIORITY              1
#define WIFI_SMARTCONFIG_TASK_STACK_SIZE            (2048 + 512)

#define WIFI_BLECONFIG_TASK_PRIORITY              1
#define WIFI_BLECONFIG_TASK_STACK_SIZE            (2048)

static int HighWaterLine = 0;
static int g_first_connected = 0;
TaskHandle_t WifiManageHandle;


#define WIFI_MANAGER_TASK_PRIORITY                  13
#define WIFI_MANAGER_TASK_STACK_SIZE                (2048 + 512)
#define SMARTCONFIG_TIMEOUT_TICK                    (60000 / portTICK_RATE_MS)
#define BLECONFIG_TIMEOUT_TICK                      (60000 / portTICK_RATE_MS)
#define WIFI_SSID CONFIG_ESP_AUDIO_WIFI_SSID
#define  WIFI_PASS CONFIG_ESP_AUDIO_WIFI_PWD

static QueueHandle_t xQueueWifi;
extern EventGroupHandle_t g_sc_event_group;
extern EventGroupHandle_t g_ble_event_group;
extern QueueHandle_t xQueWifi;
static TaskHandle_t SmartConfigHandle;
static TaskHandle_t BleConfigHandle;

#if CONFIG_ENABLE_BLE_CONFIG
extern uint8_t gl_sta_bssid[6];
extern uint8_t gl_sta_ssid[32];
extern int gl_sta_ssid_len;
#endif

static wifi_config_t sta_config = {
    .sta = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
        .bssid_set = false
    }
};

static esp_err_t save_wifi_info(char *_ssid, char *_password)
{
	int i = 0;
	int is_find_wifi = 0;
	esp_err_t ret = ESP_OK;
	DEVICE_WIFI_INFOS_T wifi_infos;
	DEVICE_WIFI_INFO_T *wifi_info = NULL;

	if (_ssid == NULL 
		|| _password == NULL
		|| strlen(_ssid) <= 0)
	{
		ESP_LOGE(WIFICONFIG_TAG, "save_wifi_info invalid params");
		return ESP_FAIL;
	}
	
	memset(&wifi_infos, 0, sizeof(wifi_infos));
	ret = get_flash_cfg(FLASH_CFG_WIFI_INFO, &wifi_infos);
	if (ret != ESP_OK)
	{
		ESP_LOGE(WIFICONFIG_TAG, "save_wifi_info get_flash_cfg failed");
		return ret;
	}

	print_wifi_infos(&wifi_infos);

	for (i=0; i < MAX_WIFI_NUM; i++)
	{
		wifi_info = &wifi_infos.wifi_info[i];
		if (strncmp(_ssid, wifi_info->wifi_ssid, sizeof(wifi_info->wifi_ssid)) == 0
			&& strncmp(_password, wifi_info->wifi_passwd, sizeof(wifi_info->wifi_passwd)) == 0)
		{
			wifi_infos.wifi_connect_index = i;
			is_find_wifi = 1;
			break;
		}
	}

	if (is_find_wifi == 0)
	{
		if (wifi_infos.wifi_storage_index < MAX_WIFI_NUM)
		{
			if (wifi_infos.wifi_storage_index == (MAX_WIFI_NUM - 1))
			{
				wifi_infos.wifi_storage_index = 0;
				wifi_infos.wifi_connect_index = 0;
				wifi_info = &wifi_infos.wifi_info[0];
			}
			else
			{
				wifi_infos.wifi_storage_index++;
				wifi_infos.wifi_connect_index = wifi_infos.wifi_storage_index;
				wifi_info = &wifi_infos.wifi_info[wifi_infos.wifi_storage_index];
			}
		}
		else
		{
			wifi_infos.wifi_storage_index = 0;
			wifi_infos.wifi_connect_index = 0;
			wifi_info = &wifi_infos.wifi_info[0];
		}

		snprintf(&wifi_info->wifi_ssid, sizeof(wifi_info->wifi_ssid), "%s", _ssid);
		snprintf(&wifi_info->wifi_passwd, sizeof(wifi_info->wifi_passwd), "%s", _password);
	}
	
	print_wifi_infos(&wifi_infos);
	
	ret = set_flash_cfg(FLASH_CFG_WIFI_INFO, &wifi_infos);
	if (ret != ESP_OK)
	{
		ESP_LOGE(WIFICONFIG_TAG, "save_wifi_info set_flash_cfg failed(%x)", ret);
		return ret;
	}

	return ret;
}

static esp_err_t wifiEvtHandlerCb(void *ctx, system_event_t *evt)
{
    WifiState g_WifiState;
    static char lastState = -1;
	static int wifi_connect_index = -1;
	DEVICE_WIFI_INFOS_T wifi_infos;
	wifi_config_t wifi_config = 
	{
	    .sta = 
		{
	        .ssid = WIFI_SSID,
	        .password = WIFI_PASS,
	        .bssid_set = false
	    }
	};
	
    if (evt == NULL) {
        return ESP_FAIL;
    }

    switch (evt->event_id) {
    case SYSTEM_EVENT_WIFI_READY:
        ESP_LOGI(WIFICONFIG_TAG, "+WIFI:READY");
        break;

    case SYSTEM_EVENT_SCAN_DONE:
        ESP_LOGI(WIFICONFIG_TAG, "+SCANDONE");
        break;

    case SYSTEM_EVENT_STA_START:
        g_WifiState = WifiState_Connecting;
        xQueueSend(xQueueWifi, &g_WifiState, 0);
        ESP_LOGI(WIFICONFIG_TAG, "+WIFI:STA_START");

        break;

    case SYSTEM_EVENT_STA_STOP:

        ESP_LOGI(WIFICONFIG_TAG, "+WIFI:STA_STOP");
        break;

    case SYSTEM_EVENT_STA_CONNECTED:
        g_WifiState = WifiState_Connected;
        xQueueSend(xQueueWifi, &g_WifiState, 0);
		g_first_connected = 1;
        ESP_LOGI(WIFICONFIG_TAG, "+JAP:WIFICONNECTED");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        if (lastState != SYSTEM_EVENT_STA_DISCONNECTED) {
            g_WifiState = WifiState_Disconnected;
         //   LedIndicatorSet(LedIndexNet, LedWorkState_NetDisconnect);
        }

        ESP_LOGI(WIFICONFIG_TAG, "+JAP:DISCONNECTED,%u,%u", 0, evt->event_info.disconnected.reason);
        EventBits_t uxBits = xEventGroupWaitBits(g_sc_event_group, SC_PAIRING, true, false, 0);
#if CONFIG_ENABLE_BLE_CONFIG
        EventBits_t uxBleBits = xEventGroupWaitBits(g_ble_event_group, BLE_CONFIGING, true, false, 0);
#endif
        // WiFi connected event
#if CONFIG_ENABLE_BLE_CONFIG

        if (!(uxBits & SC_PAIRING) && !(uxBleBits & BLE_CONFIGING)) {
#else

        if (!(uxBits & SC_PAIRING)) {
#endif

            if (lastState != SYSTEM_EVENT_STA_DISCONNECTED) {
				if (g_first_connected == 1)
				{
					player_mdware_play_tone(FLASH_MUSIC_NETWORK_DISCONNECTED);
				}
                xQueueSend(xQueueWifi, &g_WifiState, 0);
            }

			memset(&wifi_infos, 0, sizeof(wifi_infos));
			int ret = get_flash_cfg(FLASH_CFG_WIFI_INFO, &wifi_infos);
			if (ret == ESP_OK)
			{
				if (wifi_connect_index == -1)
				{
					wifi_connect_index = wifi_infos.wifi_connect_index;
				}
				else
				{
					wifi_connect_index++;
					if (wifi_connect_index >= MAX_WIFI_NUM)
					{
						wifi_connect_index = 0;
					}
				}
				
				print_wifi_infos(&wifi_infos);
				snprintf(&wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", wifi_infos.wifi_info[wifi_connect_index].wifi_ssid);
				snprintf(&wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", wifi_infos.wifi_info[wifi_connect_index].wifi_passwd);
			}
			else
			{
				ESP_LOGE(WIFICONFIG_TAG, "wifiEvtHandlerCb get_flash_cfg failed");
				memcpy(&wifi_config, &sta_config, sizeof(wifi_config));
			}

			if (strlen((char*)wifi_config.sta.ssid) <= 0)
			{
				memcpy(&wifi_config, &sta_config, sizeof(wifi_config));
			}
	
            ESP_LOGI(WIFICONFIG_TAG, "Autoconnect to Wi-Fi ssid:%s,pwd:%s.", 
            	wifi_config.sta.ssid, wifi_config.sta.password);
			
            esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
            if (ESP_FAIL == esp_wifi_connect()) 
			{
                ESP_LOGE(WIFICONFIG_TAG, "Autoconnect Wi-Fi failed");
            }
        } 
		else 
		{
            if (lastState != SYSTEM_EVENT_STA_DISCONNECTED) {
#ifdef CONFIG_ENABLE_SMART_CONFIG
                g_WifiState = WifiState_SC_Disconnected;
#elif (defined CONFIG_ENABLE_BLE_CONFIG)
                g_WifiState = WifiState_BLE_Disconnected;
#endif
                xQueueSend(xQueueWifi, &g_WifiState, 0);
            }

            ESP_LOGI(WIFICONFIG_TAG, "don't Autoconnect due to smart_config or ble config!");
        }

        lastState = SYSTEM_EVENT_STA_DISCONNECTED;
#if CONFIG_ENABLE_BLE_CONFIG
        memset(gl_sta_ssid, 0, 32);
        memset(gl_sta_bssid, 0, 6);
        gl_sta_ssid_len = 0;
#endif
        break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
        ESP_LOGI(WIFICONFIG_TAG, "+JAP:AUTHCHANGED,%d,%d",
                 evt->event_info.auth_change.new_mode,
                 evt->event_info.auth_change.old_mode);
        break;
    case SYSTEM_EVENT_STA_GOT_IP: {
        if (lastState != SYSTEM_EVENT_STA_GOT_IP) 
		{
            g_WifiState = WifiState_GotIp;
            xQueueSend(xQueueWifi, &g_WifiState, 0);
			player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNEC_SUCCESS);
        }
        wifi_config_t w_config;
        memset(&w_config, 0x00, sizeof(wifi_config_t));
        if (ESP_OK == esp_wifi_get_config(WIFI_IF_STA, &w_config)) 
		{
			if (save_wifi_info((char*)w_config.sta.ssid, (char*)w_config.sta.password) != ESP_OK)
			{
				 ESP_LOGE(WIFICONFIG_TAG, "save_wifi_info fail");
				 save_wifi_info((char*)w_config.sta.ssid, (char*)w_config.sta.password);
			}
            ESP_LOGI(WIFICONFIG_TAG, ">>>>>> Connected Wifi SSID:%s <<<<<<< \r\n", w_config.sta.ssid);
        } 
		else 
		{
            ESP_LOGE(WIFICONFIG_TAG, "Got wifi config failed");
        }
        ESP_LOGI(WIFICONFIG_TAG, "SYSTEM_EVENT_STA_GOTIP");
        lastState = SYSTEM_EVENT_STA_GOT_IP;
#if CONFIG_ENABLE_BLE_CONFIG
            esp_blufi_extra_info_t info;
            wifi_mode_t mode;
            esp_wifi_get_mode(&mode);
            memset(&info, 0, sizeof(esp_blufi_extra_info_t));
            memcpy(info.sta_bssid, gl_sta_bssid, 6);
            info.sta_bssid_set = true;
            info.sta_ssid = gl_sta_ssid;
            info.sta_ssid_len = gl_sta_ssid_len;
            esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, 0, &info);
#endif
        break;
    }
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
        ESP_LOGI(WIFICONFIG_TAG, "+WPS:SUCCEED");
        // esp_wifi_wps_disable();
        //esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
        ESP_LOGI(WIFICONFIG_TAG, "+WPS:FAILED");
        break;

    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
        ESP_LOGI(WIFICONFIG_TAG, "+WPS:TIMEOUT");
        // esp_wifi_wps_disable();
        //esp_wifi_disconnect();
        break;

    case SYSTEM_EVENT_STA_WPS_ER_PIN: {
//                char pin[9] = {0};
//                memcpy(pin, event->event_info.sta_er_pin.pin_code, 8);
//                ESP_LOGI(APP_TAG, "+WPS:PIN: [%s]", pin);
            break;
        }

    case SYSTEM_EVENT_AP_START:
        ESP_LOGI(WIFICONFIG_TAG, "+WIFI:AP_START");
        break;

    case SYSTEM_EVENT_AP_STOP:
        ESP_LOGI(WIFICONFIG_TAG, "+WIFI:AP_STOP");
        break;

    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(WIFICONFIG_TAG, "+SOFTAP:STACONNECTED,"MACSTR,
                 MAC2STR(evt->event_info.sta_connected.mac));
        break;

    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(WIFICONFIG_TAG, "+SOFTAP:STADISCONNECTED,"MACSTR,
                 MAC2STR(evt->event_info.sta_disconnected.mac));
        break;

    case SYSTEM_EVENT_AP_PROBEREQRECVED:
        ESP_LOGI(WIFICONFIG_TAG, "+PROBEREQ:"MACSTR",%d",MAC2STR(evt->event_info.ap_probereqrecved.mac),
                evt->event_info.ap_probereqrecved.rssi);
        break;
    default:
        break;
    }
    return ESP_OK;
}

//@warning: must be called by app_main task
static void WifiStartUp(void)
{
    wifi_config_t w_config;
	DEVICE_WIFI_INFOS_T wifi_infos;
	memset(&wifi_infos, 0, sizeof(wifi_infos));
	
    memset(&w_config, 0x00, sizeof(wifi_config_t));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	//Modem power save mode
	//esp_wifi_set_ps(WIFI_PS_MODEM);
    //TODO: used memorized SSID and PASSWORD
    ESP_ERROR_CHECK(esp_event_loop_init(wifiEvtHandlerCb, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	if (ESP_OK == esp_wifi_get_config(WIFI_IF_STA, &w_config)) 
	{
 		get_flash_cfg(FLASH_CFG_WIFI_INFO, &wifi_infos);
		if (wifi_infos.wifi_connect_index <= MAX_WIFI_NUM)
		{
			if(strlen(wifi_infos.wifi_info[wifi_infos.wifi_connect_index].wifi_ssid) > 0)
			{
	            snprintf(&w_config.sta.ssid, sizeof(w_config.sta.ssid), 
					"%s", wifi_infos.wifi_info[wifi_infos.wifi_connect_index].wifi_ssid);
				snprintf(&w_config.sta.password, sizeof(w_config.sta.password), 
					"%s", wifi_infos.wifi_info[wifi_infos.wifi_connect_index].wifi_passwd);
	            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &w_config));
	        }
			else
			{
	            ESP_LOGI(WIFICONFIG_TAG, "1,Connect to default Wi-Fi SSID:%s", sta_config.sta.ssid);
	            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
	        }
		}
		else
		{
			ESP_LOGI(WIFICONFIG_TAG, "2,Connect to default Wi-Fi SSID:%s", sta_config.sta.ssid);
	        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
		}
    } 
	else 
	{
        ESP_LOGE(WIFICONFIG_TAG, "get wifi config failed!");
        return;
    }
	
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
#if ENABLE_TURINGWECHAT_SERVICE
    SmartconfigSetup(SC_TYPE_ESPTOUCH_AIRKISS, true);
#else
    SmartconfigSetup(SC_TYPE_ESPTOUCH, true);
#endif
#if CONFIG_ENABLE_BLE_CONFIG
    BleconfigSetup();
#endif

}

/******************************************************************************
 * FunctionName : smartconfig_task
 * Description  : start the samrtconfig proces and call back
 * Parameters   : pvParameters
 * Returns      : none
*******************************************************************************/
static void WifiConfigTask(void *pvParameters)
{
    ESP_LOGI(WIFICONFIG_TAG, "WifiConfig task running");
    xEventGroupSetBits(g_sc_event_group, SC_PAIRING);
    esp_wifi_disconnect();
    vTaskDelay(1000);
    wifi_mode_t curMode = WIFI_MODE_NULL;
    WifiState g_WifiState;
    if ((ESP_OK == esp_wifi_get_mode(&curMode)) && (curMode == WIFI_MODE_STA)) {
        ESP_LOGD(WIFICONFIG_TAG, "Smartconfig start,heap=%d\n", esp_get_free_heap_size());
        esp_err_t res = WifiSmartConnect((TickType_t) SMARTCONFIG_TIMEOUT_TICK);
        //LedIndicatorSet(LedIndexNet, LedWorkState_NetSetting);
        switch (res) {
        case ESP_ERR_INVALID_STATE:
            ESP_LOGE(WIFICONFIG_TAG, "Already in smartconfig, please wait");
            break;
        case ESP_ERR_TIMEOUT:
            g_WifiState = WifiState_Config_Timeout;
            xQueueSend(xQueueWifi, &g_WifiState, 0);
            ESP_LOGI(WIFICONFIG_TAG, "Smartconfig timeout");
			player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNEC_FAILURE);
            break;
        case ESP_FAIL:
            ESP_LOGI(WIFICONFIG_TAG, "Smartconfig failed, please try again");
			player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNEC_FAILURE);
            break;
        default:
            break;
        }
    } else {
        ESP_LOGE(WIFICONFIG_TAG, "Invalid wifi mode");
    }
#if EN_STACK_TRACKER
    ESP_LOGI("STACK", "WifiConfigTask %d\n\n\n", uxTaskGetStackHighWaterMark(SmartConfigHandle));
#endif
    vTaskDelete(NULL);
}

//public interface
void WifiSmartConfig(WifiManager *manager)
{
    //configASSERT(manager);
    if(xTaskCreate(WifiConfigTask,
                   "WifiConfigTask",
                   WIFI_SMARTCONFIG_TASK_STACK_SIZE,
                   NULL,
                   WIFI_SMARTCONFIG_TASK_PRIORITY,
                   &SmartConfigHandle) != pdPASS) {

        ESP_LOGE(WIFICONFIG_TAG, "ERROR creating WifiConfigTask task! Out of memory?");
    }
}

#if CONFIG_ENABLE_BLE_CONFIG
/******************************************************************************
 * FunctionName : BleConfigTask
 * Description  : start the Bleconfig proces and call back
 * Parameters   : pvParameters
 * Returns      : none
*******************************************************************************/
static void BleConfigTask(void *pvParameters)
{
    ESP_LOGI(WIFICONFIG_TAG, "BleConfig task running");
    wifi_mode_t curMode = WIFI_MODE_NULL;
    ESP_LOGD(WIFICONFIG_TAG, "BleConfig start,heap=%d\n", esp_get_free_heap_size());
    esp_wifi_disconnect();
    vTaskDelay(1000);
    WifiState g_WifiState;
    esp_err_t res = BleConfigStart((TickType_t) BLECONFIG_TIMEOUT_TICK);
    //LedIndicatorSet(LedIndexNet, LedWorkState_NetSetting);
    switch (res) {
    case ESP_ERR_INVALID_STATE:
        ESP_LOGE(WIFICONFIG_TAG, "Already in BleConfig, please wait");
        break;
    case ESP_ERR_TIMEOUT:
        esp_bluedroid_disable();
        esp_bluedroid_deinit();
        g_WifiState = WifiState_Config_Timeout;
        xQueueSend(xQueueWifi, &g_WifiState, 0);
        ESP_LOGI(WIFICONFIG_TAG, "BleConfig timeout");
        break;
    case ESP_FAIL:
        esp_bluedroid_disable();
        esp_bluedroid_deinit();
        ESP_LOGI(WIFICONFIG_TAG, "BleConfig failed, please try again");
        break;
    default:
        break;
    }
    vTaskDelete(NULL);
}

//public interface
static void WifiBleConfig(WifiManager *manager)
{
    //configASSERT(manager);
    if(xTaskCreate(BleConfigTask,
                   "BleConfigTask",
                   WIFI_BLECONFIG_TASK_STACK_SIZE,
                   NULL,
                   WIFI_BLECONFIG_TASK_PRIORITY,
                   &BleConfigHandle) != pdPASS) {

        ESP_LOGE(WIFICONFIG_TAG, "ERROR creating BleConfigTask task! Out of memory?");
    }
}

static void WifiBleConfigStop(WifiManager *manager)
{
    esp_bluedroid_status_t status = esp_bluedroid_get_status();
    if(status >= ESP_BLUEDROID_STATUS_INITIALIZED) {
        esp_bluedroid_deinit();
    }
    if(status == ESP_BLUEDROID_STATUS_ENABLED) {
        esp_bluedroid_disable();
    }

}
#endif
static void ProcessWifiEvent(WifiManager *manager, void *evt)
{
    WifiState state = *((WifiState *) evt);
    DeviceNotifyMsg msg = DEVICE_NOTIFY_WIFI_UNDEFINED;
    switch (state) {
    case WifiState_GotIp:
        msg = DEVICE_NOTIFY_WIFI_GOT_IP;
        break;
    case WifiState_Disconnected:
        msg = DEVICE_NOTIFY_WIFI_DISCONNECTED;
        break;
    case WifiState_SC_Disconnected:
        msg = DEVICE_NOTIFY_WIFI_SC_DISCONNECTED;
        break;
    case WifiState_BLE_Disconnected:
        msg = DEVICE_NOTIFY_WIFI_BLE_DISCONNECTED;
        break;
    case WifiState_Config_Timeout:
        msg = DEVICE_NOTIFY_WIFI_SETTING_TIMEOUT;
        break;
    case WifiState_Connecting:
    case WifiState_Connected:
    case WifiState_ConnectFailed:
    default:
        break;
    }

    if (msg != DEVICE_NOTIFY_WIFI_UNDEFINED) {
        manager->Based.notify((struct DeviceController *) manager->Based.controller, DEVICE_NOTIFY_TYPE_WIFI, &msg, sizeof(DeviceNotifyMsg));
    }
}

static void WifiManagerEvtTask(void *pvParameters)
{
    WifiManager *manager = (WifiManager *) pvParameters;
    WifiState state = WifiState_Unknow;
    while (1) {
        if (xQueueReceive(xQueueWifi, &state, portMAX_DELAY)) {
            ProcessWifiEvent(manager, &state);
#if EN_STACK_TRACKER
            if(uxTaskGetStackHighWaterMark(WifiManageHandle) > HighWaterLine){
                HighWaterLine = uxTaskGetStackHighWaterMark(WifiManageHandle);
                ESP_LOGI("STACK", "%s %d\n\n\n", __func__, HighWaterLine);
            }
#endif
        }
    }
    vTaskDelete(NULL);
}


static int WifiConfigActive(DeviceManager *self)
{
    ESP_LOGI(WIFICONFIG_TAG, "WifiConfigActive, freemem %d", esp_get_free_heap_size());
    WifiManager *manager = (WifiManager *) self;
    xQueueWifi = xQueueCreate(WIFI_EVT_QUEUE_LEN, sizeof(WifiState));
    configASSERT(xQueueWifi);
	vTaskDelay(4*1000);
	player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECTING);
	vTaskDelay(3*1000);
	WifiStartUp();
	
    if (xTaskCreatePinnedToCore(WifiManagerEvtTask,
                                "WifiManagerEvtTask",
                                WIFI_MANAGER_TASK_STACK_SIZE,
                                manager,
                                WIFI_MANAGER_TASK_PRIORITY,
                                &WifiManageHandle, xPortGetCoreID()) != pdPASS) {
        ESP_LOGE(WIFICONFIG_TAG, "Error create WifiManagerTask");
        return -1;
    }
    return 0;
}

static void WifiConfigDeactive(DeviceManager *self)
{
    ESP_LOGI(WIFICONFIG_TAG, "WifiConfigStop\r\n");

}

WifiManager *WifiConfigCreate(struct DeviceController *controller)
{
    if (!controller)
        return NULL;
    ESP_LOGI(WIFICONFIG_TAG, "WifiConfigCreate\r\n");
    WifiManager *wifi = (WifiManager *) calloc(1, sizeof(WifiManager));
    ESP_ERROR_CHECK(!wifi);

    InitManager((DeviceManager *) wifi, controller);

    wifi->Based.active = WifiConfigActive;
    wifi->Based.deactive = WifiConfigDeactive;
    wifi->Based.notify = WifiEvtNotify;  //TODO
    wifi->smartConfig = WifiSmartConfig;  //TODO
#if CONFIG_ENABLE_BLE_CONFIG
    wifi->bleConfig = WifiBleConfig;  //TODO
    wifi->bleConfigStop = WifiBleConfigStop;
#endif
    return wifi;
}

#else

WifiManager *WifiConfigCreate(struct DeviceController *controller)
{
    return NULL;
}

#endif
