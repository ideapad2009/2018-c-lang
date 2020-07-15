#ifndef _NOTIFY_HELPER_H_
#define _NOTIFY_HELPER_H_

struct DeviceController;

enum DeviceNotifyType;

void SDCardEvtNotify(struct DeviceController *controller, enum DeviceNotifyType type, void *data, int len);


void TouchpadEvtNotify(struct DeviceController *controller, enum DeviceNotifyType type, void *data, int len);

void WifiEvtNotify(struct DeviceController *controller, enum DeviceNotifyType type, void *data, int len);

void AuxInEvtNotify(struct DeviceController *controller, enum DeviceNotifyType type, void *data, int len);

#endif
