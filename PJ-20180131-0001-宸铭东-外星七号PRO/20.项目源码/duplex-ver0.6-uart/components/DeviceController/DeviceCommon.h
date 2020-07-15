#ifndef __DEVICE_COMMON_H__
#define __DEVICE_COMMON_H__

typedef enum DeviceNotifyType {
    DEVICE_NOTIFY_TYPE_SDCARD,
    DEVICE_NOTIFY_TYPE_TOUCH,
    DEVICE_NOTIFY_TYPE_WIFI,
    DEVICE_NOTIFY_TYPE_AUXIN,

} DeviceNotifyType;

typedef int DeviceNotifyMsg;

typedef enum DeviceNotifySDCardMsg {
    DEVICE_NOTIFY_SD_UNDEFINED,
    DEVICE_NOTIFY_SD_MOUNTED,
    DEVICE_NOTIFY_SD_UNMOUNTED,
    DEVICE_NOTIFY_SD_MOUNT_FAILED,
} DeviceNotifySDCardMsg;

typedef enum DeviceNotifyTouchMsg {
    DEVICE_NOTIFY_KEY_UNDEFINED,
    //vol+
    DEVICE_NOTIFY_KEY_VOL_UP,
    DEVICE_NOTIFY_KEY_PREV,
    //vol-
    DEVICE_NOTIFY_KEY_VOL_DOWN,
    DEVICE_NOTIFY_KEY_NEXT,
    //set
    DEVICE_NOTIFY_KEY_WIFI_SET,
    //play
    DEVICE_NOTIFY_KEY_PLAY,
    //rec
    DEVICE_NOTIFY_KEY_REC,
    DEVICE_NOTIFY_KEY_REC_QUIT,
    //mode
    DEVICE_NOTIFY_KEY_MODE,
    DEVICE_NOTIFY_KEY_BT_WIFI_SWITCH,
    DEVICE_NOTIFY_KEY_AIWIFI_WECHAT_SWITCH,
} DeviceNotifyTouchMsg;

typedef enum DeviceNotifyWifiMsg {
    DEVICE_NOTIFY_WIFI_UNDEFINED,
    DEVICE_NOTIFY_WIFI_GOT_IP,
    DEVICE_NOTIFY_WIFI_DISCONNECTED,
    DEVICE_NOTIFY_WIFI_SETTING_TIMEOUT,
    DEVICE_NOTIFY_WIFI_SC_DISCONNECTED,//smart config disconnect
    DEVICE_NOTIFY_WIFI_BLE_DISCONNECTED,// ble config disconnect
} DeviceNotifyWifiMsg;

typedef enum DeviceNotifyAuxInMsg {
    DEVICE_NOTIFY_AUXIN_UNDEFINED,
    DEVICE_NOTIFY_AUXIN_INSERTED,
    DEVICE_NOTIFY_AUXIN_REMOVED,
} DeviceNotifyAuxInMsg;

//
typedef struct DeviceNotification {
    void *receiver;
    DeviceNotifyType type;
    void *data;
    int len;
} DeviceNotification;

#endif
