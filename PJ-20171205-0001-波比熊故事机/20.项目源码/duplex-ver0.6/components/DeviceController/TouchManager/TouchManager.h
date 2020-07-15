#ifndef _TOUCH_MANAGER_H_
#define _TOUCH_MANAGER_H_
#include "DeviceManager.h"

struct DeviceController;

typedef struct TouchManager TouchManager;

/* Implements DeviceManager */
struct TouchManager {
    /* extend */
    DeviceManager Based;
    /* private */
};

TouchManager *TouchManagerCreate(struct DeviceController *controller);


#endif
