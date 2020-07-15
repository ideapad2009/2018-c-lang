#ifndef _WIFI_MANAGER_H_
#define _WIFI_MANAGER_H_
#include "DeviceManager.h"

typedef struct
{
	int wifi_start_link_flag;	//记录WiFi是否开始进行用户名和密码验证，0为否，1为是。
	long int connect_wifi_time;	//记录获取用户名后开始联网的时间点
	int auto_re_connect_times;	//回连次数记录
}WIFI_OTHERS_FLAG_T;

typedef enum
{
	WIFI_MODE_WAITING_APP,		//WiFi等待APP连接中
	WIFI_MODE_AUTO_RE_CONNECT,	//回连保存的网络
	WIFI_MODE_CHOICE,			//选择等待APP或回连
	WIFI_MODE_OFF,				//WiFi关闭
}WIFI_MODE_T; 

typedef enum
{
	WIFI_STATE_CONNECTED,		//网络已连接
	WIFI_STATE_DISCONNECT,		//网络已断开
}WIFI_STATE_T; 

typedef enum
{
	WIFI_AUTO_RE_CONNECT_STATE_ENABLE,		//可以自动回连网络
	WIFI_AUTO_RE_CONNECT_STATE_DISABLE,		//不能自动回连网络
}WIFI_AUTO_RE_CONNECT_STATE_T; 

typedef enum {
    SMARTCONFIG_NO,
    SMARTCONFIG_YES
} SmartconfigStatus;

typedef enum {
    WifiState_Unknow,
    WifiState_Config_Timeout,
    WifiState_Connecting,
    WifiState_Connected,
    WifiState_Disconnected,
    WifiState_ConnectFailed,
    WifiState_GotIp,
    WifiState_SC_Disconnected,//smart config Disconnected
    WifiState_BLE_Disconnected,
} WifiState;

struct DeviceController;

typedef struct WifiManager WifiManager;

/* Implements DeviceManager */
struct WifiManager {
    /*extend*/
    DeviceManager Based;
    void (*smartConfig)(WifiManager* manager);
    void (*bleConfig)(WifiManager* manager);
    void (*bleConfigStop)(WifiManager* manager);
    /* private */
};

WifiManager* WifiConfigCreate(struct DeviceController* controller);

/*增加网络模式配置任务*/
void wifi_mode_config_create();

/*启动配网*/
void wifi_smart_connect_start();

/*启动自动回连保存过的网络*/
void wifi_auto_re_connect_start();

/*断网时自动选择重连或等待APP模式*/
void choice_reconnect_waitapp_mode_start();

/*关闭配网并断网*/
void wifi_off_start();

/*记录当前wifi模式的状态*/
void set_wifi_mode_state(WIFI_MODE_T wifi_mode);

/*获取wifi模式的状态*/
WIFI_MODE_T get_wifi_mode_state();

/*记录为开始连接，记号设为1*/
void set_wifi_start_link_flag();

/*获取开始联网时间点*/
void get_connect_wifi_time();

/*联网出错处理*/
void connect_wifi_error_manage();

#endif
