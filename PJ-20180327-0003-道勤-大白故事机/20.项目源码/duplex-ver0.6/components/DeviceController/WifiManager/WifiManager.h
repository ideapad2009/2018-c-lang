#ifndef _WIFI_MANAGER_H_
#define _WIFI_MANAGER_H_
#include "DeviceManager.h"

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

#endif
