#ifndef _DEVICE_CONTROLLER_H_
#define _DEVICE_CONTROLLER_H_

struct DeviceManager;

struct DeviceNotification;

struct PlaylistInfo;

struct MediaControl;

typedef struct DeviceController DeviceController;

/* Builder */
struct DeviceController {
    void *instance;
    struct DeviceManager *wifi;
    struct DeviceManager *touch;
    struct DeviceManager *sdcard;
    struct DeviceManager *auxIn;
    void (*enableWifi) (DeviceController *controller);
    void (*enableTouch) (DeviceController *controller);
    void (*enableSDcard) (DeviceController *controller);
    void (*enableAuxIn) (DeviceController *controller);
    void (*activeManagers) (DeviceController *controller);

    /*Wifi interface*/
    void (*wifiSmartConfig) (DeviceController *controller);
    /*Ble config*/
    void (*wifiBleConfig) (DeviceController *controller);
    void (*wifiBleConfigStop) (DeviceController *controller);
};


DeviceController *DeviceCtrlCreate(struct MediaControl *ctrl);


#endif
