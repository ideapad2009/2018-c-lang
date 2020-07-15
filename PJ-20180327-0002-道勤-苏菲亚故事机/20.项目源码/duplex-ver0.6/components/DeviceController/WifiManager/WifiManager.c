#include "auto_play_service.h"
#include "device_api.h"
#include "DeviceCommon.h"
#include "EspAudio.h"
#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "flash_config_manage.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "free_talk.h"
#include "led.h"
#include "NotifyHelper.h"
#include "player_middleware.h"
#include "sdkconfig.h"
#include "toneuri.h"
#include "userconfig.h"
#include "WifiManager.h"
#include "wifismartconfig.h"
#include "wifibleconfig.h"
#include <stdio.h>
#include <string.h>

#define WIFICONFIG_TAG                      "WIFI_CONFIG"
#define WIFI_EVT_QUEUE_LEN                  4
#define WIFI_SMARTCONFIG_TASK_PRIORITY      1
#define WIFI_SMARTCONFIG_TASK_STACK_SIZE    (2048 + 512)
#define WIFI_MANAGER_TASK_PRIORITY          1//13
#define WIFI_MANAGER_TASK_STACK_SIZE        (2048 + 512)
#define SMARTCONFIG_TIMEOUT_TICK            (60 * 1000 / portTICK_RATE_MS)//1 min
#define WIFI_SSID 							CONFIG_ESP_AUDIO_WIFI_SSID
#define WIFI_PASS 							CONFIG_ESP_AUDIO_WIFI_PWD

typedef struct
{
	WIFI_OTHERS_FLAG_T wifi_flag;	//WiFi其它记号
	WIFI_MODE_T wifi_mode;			//网络模式记录
	WIFI_STATE_T wifi_state;		//网络状态记录
	xQueueHandle msg_queue;			//消息队列
}WIFI_MODE_HANDLE_T;

#if 0
typedef struct
{
	WIFI_AUTO_RE_CONNECT_STATE_T auto_re_connect_state;
	xQueueHandle msg_queue;
}WIFI_AUTO_RE_CONNECT_HANDLE_T;
#endif

static int HighWaterLine = 0;
static int g_first_connected = 0;
TaskHandle_t WifiManageHandle;
static WIFI_MODE_HANDLE_T *g_wifi_mode_handle;
//static WIFI_AUTO_RE_CONNECT_HANDLE_T *g_wifi_auto_re_connect_handle;

static QueueHandle_t xQueueWifi;
extern EventGroupHandle_t g_sc_event_group;
extern QueueHandle_t xQueWifi;
static TaskHandle_t SmartConfigHandle;
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

	for (i=1; i < MAX_WIFI_NUM; i++)
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
				wifi_infos.wifi_storage_index = 1;
				wifi_infos.wifi_connect_index = 1;
				wifi_info = &wifi_infos.wifi_info[1];
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
			wifi_infos.wifi_storage_index = 1;
			wifi_infos.wifi_connect_index = 1;
			wifi_info = &wifi_infos.wifi_info[1];
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
	static int auto_connect_flag = 0;
#if 0
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
#endif

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
        if (lastState != SYSTEM_EVENT_STA_DISCONNECTED)
		{
            g_WifiState = WifiState_Disconnected;
        }
        ESP_LOGI(WIFICONFIG_TAG, "+JAP:DISCONNECTED,%u,%u", 0, evt->event_info.disconnected.reason);
		if (get_wifi_mode_state() != WIFI_MODE_WAITING_APP)
        {
            if (lastState != SYSTEM_EVENT_STA_DISCONNECTED) 
			{
				if (g_first_connected == 1)//网络至少连接上一次才执行以下操作
				{
					if (is_free_talk_running())
					{
						free_talk_stop();//关闭录音
						auto_play_stop();//停止自动播放
					}
					if (get_wifi_mode_state() != WIFI_MODE_WAITING_APP)
					{
						auto_connect_flag = 0;
						player_mdware_play_tone(FLASH_MUSIC_NETWORK_DISCONNECTED);
						vTaskDelay(3*1000);
					}
				}
                xQueueSend(xQueueWifi, &g_WifiState, 0);
            }
			if (auto_connect_flag == 0)
			{
				wifi_auto_re_connect_start();//自动回连
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
			g_wifi_mode_handle->wifi_flag.auto_re_connect_times = 0;//网络连接，回连次数重置为0
			g_wifi_mode_handle->wifi_flag.wifi_start_link_flag = 0;	//网络连接，重置连接记号为0
			//set_wifi_state_flag(WIFI_STATE_CONNECTED);//网络已连接记号
			auto_connect_flag = 1;
			auto_play_stop();
			vTaskDelay(200);
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

}

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
	vTaskDelay(2*1000);
	WifiStartUp();
    if (xTaskCreate(WifiManagerEvtTask,
                    "WifiManagerEvtTask",
                    512*4,
                    manager,
                    WIFI_MANAGER_TASK_PRIORITY,
                    &WifiManageHandle) != pdPASS) {

        ESP_LOGE(WIFICONFIG_TAG, "ERROR creating WifiManagerEvtTask task! Out of memory?");
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

	wifi_mode_config_create();//增加网络连接模式配置任务
    wifi->Based.active = WifiConfigActive;
    wifi->Based.deactive = WifiConfigDeactive;
    wifi->Based.notify = WifiEvtNotify;  //TODO
//    wifi->smartConfig = WifiSmartConfig;  //TODO	
	
    return wifi;
}

/*******************************************************************************
*								手 动 开 关 网 络 								   *
*******************************************************************************/

//此接口会自动配合APP连接网络,未联网状态下10个小时内将会一直处于等待APP配网状态
void wifi_smart_connect()
{
    ESP_LOGI(WIFICONFIG_TAG, "WifiConfig task running");
    xEventGroupSetBits(g_sc_event_group, SC_PAIRING);
    esp_wifi_disconnect();
    wifi_mode_t curMode = WIFI_MODE_NULL;
    WifiState g_WifiState;
    if ((ESP_OK == esp_wifi_get_mode(&curMode)) && (curMode == WIFI_MODE_STA)) {
        ESP_LOGD(WIFICONFIG_TAG, "Smartconfig start,heap=%d\n", esp_get_free_heap_size());
        esp_err_t res = WifiSmartConnect((TickType_t) SMARTCONFIG_TIMEOUT_TICK);
		switch (res) 
		{
	        case ESP_ERR_INVALID_STATE:
	        {
	        	ESP_LOGE(WIFICONFIG_TAG, "Already in smartconfig, please wait");
	            break;
			}
			case ESP_OK:
			{
				break;
			}
	        case ESP_FAIL:
	        {
	        	if (get_wifi_mode_state() != WIFI_MODE_OFF)	
	        	{
	            	ESP_LOGE(WIFICONFIG_TAG, "Smartconfig failed, please try again");
					player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNEC_FAILURE);
	        	}
	            break;
	        }
	        default:
	        {
	        	if (get_wifi_mode_state() != WIFI_MODE_OFF)	
	        	{
	        		player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNEC_FAILURE);
	        	}
	            break;
	        }
        }
    }
}

//此接口会回连之前保存的网络
void wifi_auto_re_connect()
{
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
		
	if (get_wifi_mode_state() == WIFI_MODE_AUTO_RE_CONNECT)//判断是否启动WiFi，以决定是否进行网络回连
	{
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
        if (esp_wifi_connect() != ESP_OK) 
		{
            ESP_LOGE(WIFICONFIG_TAG, "Autoconnect Wi-Fi failed");
        }
	}
}

//此接口会断开网络，并停止配网
void wifi_off()
{
	if (is_free_talk_running())//是否启动连续对话
	{
		free_talk_stop();//关闭录音
		auto_play_stop();//停止自动播放
	}
    if (esp_wifi_disconnect() != ESP_OK)//使网络断开
    {
    	esp_wifi_disconnect();
    }
	/*删除原因：调用esp_wifi_stop()会导致SD卡播不出来，原因不明
	if (esp_wifi_stop() != ESP_OK)//停止wifi
	{
		esp_wifi_stop();
	}
	*/
	esp_smartconfig_stop();//停止配网
}

//断网时自动选择重连或等待APP模式
void choose_reconnect_waitapp_mode()
{
	if (g_wifi_mode_handle->wifi_flag.auto_re_connect_times <= MAX_WIFI_NUM)
	{
		g_wifi_mode_handle->wifi_flag.auto_re_connect_times++;
		wifi_auto_re_connect_start();
	}
	else
	{
		g_wifi_mode_handle->wifi_flag.auto_re_connect_times = 0;
		wifi_smart_connect_start();
		player_mdware_play_tone(FLASH_MUSIC_PLEASE_RECONNECT_THE_NETWORK);
	}
}

//网络模式事件处理任务
void wifi_mode_evt_task(WIFI_MODE_T p_msg)
{
	while (1)
	{
		if (xQueueReceive(g_wifi_mode_handle->msg_queue, &p_msg, portMAX_DELAY) == pdPASS) 
		{
			ESP_LOGE(WIFICONFIG_TAG, "******************p_msg = [%d]***************", p_msg);
			switch(p_msg)
			{
				case WIFI_MODE_WAITING_APP:
				{
					wifi_smart_connect();				
					break;
				}
				case WIFI_MODE_AUTO_RE_CONNECT:
				{
					wifi_auto_re_connect();
					break;
				}
				case WIFI_MODE_CHOICE:
				{
					choose_reconnect_waitapp_mode();
					break;
				}
				case WIFI_MODE_OFF:
				{
					wifi_off();
					break;
				}
				default:
					break;
			}
		}
	}
	vTaskDelete(NULL);
}

//增加网络模式配置任务
void wifi_mode_config_create()
{
	g_wifi_mode_handle = (WIFI_MODE_HANDLE_T *)esp32_malloc(sizeof(WIFI_MODE_HANDLE_T));
	if (g_wifi_mode_handle == NULL)
	{
		ESP_LOGE(WIFICONFIG_TAG, "wifi_mode_handle failed, out of memory");
		return;
	}
	memset((char*)g_wifi_mode_handle, 0, sizeof(WIFI_MODE_HANDLE_T));
	g_wifi_mode_handle->msg_queue = xQueueCreate(2, sizeof(char *));
	g_wifi_mode_handle->wifi_flag.auto_re_connect_times = 0;	//回连次数，初始化为0次
	g_wifi_mode_handle->wifi_flag.connect_wifi_time = 0;		//初始化时间为0
	g_wifi_mode_handle->wifi_flag.wifi_start_link_flag = 0;		//初始化验证状态为否
	set_wifi_mode_state(WIFI_MODE_CHOICE);//初始化连网模式为选择等待APP或回连模式
	
	if (xTaskCreate(wifi_mode_evt_task,
                    "wifi_mode_evt_task",
                    512*5,
                    g_wifi_mode_handle,
                    4,
                    NULL) != pdPASS) {

        ESP_LOGE(WIFICONFIG_TAG, "ERROR creating wifi_mode_task task! Out of memory?");
    }
}

//防止自动配网无法正常关闭
void prevent_smart_connect_not_close()
{
	set_event_group_stop_bits();//设置g_sc_event_group的数据为SC_STOP_REQ_EVT,使WifiSmartConnect()函数可以退出，即停止自动配网
	//esp_smartconfig_stop();//停止配网
}

//启动配网
void wifi_smart_connect_start()
{
	printf("********************** in wifi_smart_connect_start ! ********************\n");
	
	WIFI_MODE_T p_msg = WIFI_MODE_WAITING_APP;
	
	set_wifi_mode_state(WIFI_MODE_WAITING_APP);
//	led_ctrl_set_mode(LED_CTRL_U_D_EYE_FLASHING_FAST);//启动配网，上下眼同时闪烁
	prevent_smart_connect_not_close();//防止自动配网无法正常关闭
	if (xQueueSend(g_wifi_mode_handle->msg_queue, &p_msg, 0) != pdPASS)
	{
		if (xQueueSend(g_wifi_mode_handle->msg_queue, &p_msg, 0))
		{
			ESP_LOGE(WIFICONFIG_TAG, "wifi_smart_connect_start msg_queue second send faild!");
		}
	}
}

//启动自动回连保存过的网络
void wifi_auto_re_connect_start()
{
	printf("********************** in wifi_auto_re_connect_start ! ********************\n");
	
	WIFI_MODE_T p_msg = WIFI_MODE_AUTO_RE_CONNECT;
	
	set_wifi_mode_state(WIFI_MODE_AUTO_RE_CONNECT);
//	led_ctrl_set_mode(LED_CTRL_UP_EYE_LIGHT);//检测网络，只让上眼灯常亮
//	led_ctrl_set_mode(LED_CTRL_DOWN_EYE_CLOSE);//检测网络，下眼灯灭
	prevent_smart_connect_not_close();//防止自动配网无法正常关闭
	if (xQueueSend(g_wifi_mode_handle->msg_queue, &p_msg, 0) != pdPASS)
	{
		if (xQueueSend(g_wifi_mode_handle->msg_queue, &p_msg, 0))
		{
			ESP_LOGE(WIFICONFIG_TAG, "wifi_smart_connect_start msg_queue second send faild!");
		}
	}
}

//关闭配网并断网
void wifi_off_start()
{
	printf("********************** in wifi_off_start ! ********************\n");
	
	WIFI_MODE_T p_msg = WIFI_MODE_OFF;
	
	set_wifi_mode_state(WIFI_MODE_OFF);	
//	led_ctrl_set_mode(LED_CTRL_U_D_EYE_ALTERANTE_SCINTILLATION);//无网络上下眼交替闪烁
	prevent_smart_connect_not_close();//防止自动配网无法正常关闭
	if (xQueueSend(g_wifi_mode_handle->msg_queue, &p_msg, 0) != pdPASS)
	{
		if (xQueueSend(g_wifi_mode_handle->msg_queue, &p_msg, 0))
		{
			ESP_LOGE(WIFICONFIG_TAG, "wifi_off_start msg_queue second send faild!");
		}
	}
}

//自动选择重连或等待APP模式
void choice_reconnect_waitapp_mode_start()
{
	printf("********************** in choice_reconnect_waitapp_mode_start ! ********************\n");
	
	WIFI_MODE_T p_msg = WIFI_MODE_CHOICE;
	
	set_wifi_mode_state(WIFI_MODE_CHOICE);
	prevent_smart_connect_not_close();//防止自动配网无法正常关闭
	if (xQueueSend(g_wifi_mode_handle->msg_queue, &p_msg, 0) != pdPASS)
	{
		if (xQueueSend(g_wifi_mode_handle->msg_queue, &p_msg, 0))
		{
			ESP_LOGE(WIFICONFIG_TAG, "choice_reconnect_waitapp_mode_start msg_queue second send faild!");
		}
	}
}

//记录当前wifi模式的状态
void set_wifi_mode_state(WIFI_MODE_T wifi_mode)
{
	g_wifi_mode_handle->wifi_mode = wifi_mode;
}

//获取wifi模式的状态
WIFI_MODE_T get_wifi_mode_state()
{
	WIFI_MODE_T wifi_mode = g_wifi_mode_handle->wifi_mode;
	return wifi_mode;
}

//记录为开始连接，记号设为1
void set_wifi_start_link_flag()
{
	g_wifi_mode_handle->wifi_flag.wifi_start_link_flag = 1;
}

//获取开始联网时间点
void get_connect_wifi_time()
{
	g_wifi_mode_handle->wifi_flag.connect_wifi_time = get_time_of_day();
}

//联网出错处理
void connect_wifi_error_manage()
{
	if (g_wifi_mode_handle->wifi_flag.wifi_start_link_flag == 1)	//判断是否已进入连接状态，1为已进入连接
	{
		long int current = get_time_of_day();	//获取当前时间
		printf("***************************** current = [%ld], connect_wifi_time = [%ld] *************************\n", current, g_wifi_mode_handle->wifi_flag.connect_wifi_time);
		
		if ((current - g_wifi_mode_handle->wifi_flag.connect_wifi_time) > 10*1000)//计算WiFi连接时长，做超时计算，单位毫秒
		{
			g_wifi_mode_handle->wifi_flag.wifi_start_link_flag = 0;	//重置开始连接记号为0
			//choice_reconnect_waitapp_mode_start();					//自动选择重连或等待APP模式
		}
	}
}

#if 0
//记录当前wifi状态
static void set_wifi_state_flag(WIFI_STATE_T wifi_state)
{
	g_wifi_mode_handle->wifi_state = wifi_state;
}

//获取wifi状态
static WIFI_STATE_T get_wifi_state_flag()
{
	WIFI_STATE_T wifi_state = g_wifi_mode_handle->wifi_state;
	return wifi_state;
}
#endif

#if 0
/*****************************************************************
*						 网 络 自 动 回 连 							 *
*****************************************************************/

//网络自动回连任务
void wifi_auto_re_connect_task(WIFI_AUTO_RE_CONNECT_STATE_T p_msg)
{
	while (1)
	{
		if (xQueueReceive(g_wifi_auto_re_connect_handle->msg_queue, &p_msg, portMAX_DELAY) == pdPASS) 
		{
			ESP_LOGE(WIFICONFIG_TAG, "******************p_msg = [%d]***************", p_msg);
			switch(p_msg)
			{
				case WIFI_AUTO_RE_CONNECT_STATE_ENABLE:
				{
					break;
				}
				case WIFI_AUTO_RE_CONNECT_STATE_DISABLE:
				{
					break;
				}
				default:
					break;
			}
		}
	}
	vTaskDelete(NULL);
}

//增加网络自动回连任务
void wifi_auto_re_connect_config_create()
{
	g_wifi_auto_re_connect_handle = (WIFI_AUTO_RE_CONNECT_HANDLE_T *)esp32_malloc(sizeof(WIFI_AUTO_RE_CONNECT_HANDLE_T));
	if (g_wifi_auto_re_connect_handle == NULL)
	{
		ESP_LOGE(WIFICONFIG_TAG, "g_wifi_auto_re_connect_handle failed, out of memory");
		return;
	}
	memset((char*)g_wifi_auto_re_connect_handle, 0, sizeof(WIFI_AUTO_RE_CONNECT_HANDLE_T));
	g_wifi_auto_re_connect_handle->msg_queue = xQueueCreate(2, sizeof(char *));
	
	if (xTaskCreate(wifi_auto_re_connect_task,
                    "wifi_auto_re_connect_task",
                    512*5,
                    g_wifi_auto_re_connect_handle,
                    4,
                    NULL) != pdPASS) {

        ESP_LOGE(WIFICONFIG_TAG, "ERROR creating wifi_auto_re_connect_task task! Out of memory?");
    }
}

//启动自动回连保存过的网络
void wifi_auto_re_connect_start()
{
	WIFI_AUTO_RE_CONNECT_STATE_T p_msg = WIFI_AUTO_RE_CONNECT_STATE_ENABLE;
	
	if (xQueueSend(g_wifi_auto_re_connect_handle->msg_queue, &p_msg, 0) != pdPASS)
	{
		if (xQueueSend(g_wifi_auto_re_connect_handle->msg_queue, &p_msg, 0))
		{
			ESP_LOGE(WIFICONFIG_TAG, "wifi_auto_re_connect_start msg_queue second send faild!");
		}
	}
}

//关闭自动回连
void wifi_auto_re_connect_off_start()
{
	WIFI_AUTO_RE_CONNECT_STATE_T p_msg = WIFI_AUTO_RE_CONNECT_STATE_DISABLE;
	
	if (xQueueSend(g_wifi_auto_re_connect_handle->msg_queue, &p_msg, 0) != pdPASS)
	{
		if (xQueueSend(g_wifi_auto_re_connect_handle->msg_queue, &p_msg, 0))
		{
			ESP_LOGE(WIFICONFIG_TAG, "wifi_auto_re_connect_off_start msg_queue second send faild!");
		}
	}
}
#endif
