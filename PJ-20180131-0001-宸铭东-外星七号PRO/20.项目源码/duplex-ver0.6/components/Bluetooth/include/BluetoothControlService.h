#ifndef _BLUETOOTH_CTRL_SERVICE_H_
#define _BLUETOOTH_CTRL_SERVICE_H_
#include "MediaService.h"


typedef struct BluetoothControlService { //extern from TreeUtility
    /*relation*/
    MediaService Based;
} BluetoothControlService;

BluetoothControlService *BluetoothControlCreate();
#endif
