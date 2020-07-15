/**
 * Interface
 */

#ifndef _DEVICE_MANAGER_H_
#define _DEVICE_MANAGER_H_

enum DeviceNotifyType;

struct DeviceController;

typedef struct DeviceManager DeviceManager;

/* Interface */
struct DeviceManager {
    void *instance;
    struct DeviceController *controller;  /* backward reference */
    char *tag;  //XXX: is this needed?? good for debug and visualization
    int (*active)(DeviceManager *manager);
    void (*deactive)(DeviceManager *manager);
    void (*notify)(struct DeviceController *controller, enum DeviceNotifyType type, void *data, int len);
    void (*pauseMedia)(DeviceManager *controller);
    void (*resumeMedia)(DeviceManager *controller);
};

void InitManager(DeviceManager *manager, struct DeviceController *controller);

#endif
