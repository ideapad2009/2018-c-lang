#ifndef _AUX_IN_MANAGER_H_
#define _AUX_IN_MANAGER_H_
#include "DeviceManager.h"

typedef struct AuxInManager AuxInManager;

struct DeviceController;

/* Extends DeviceManager */
struct AuxInManager {
    /* extend */
    DeviceManager Based;
    /* private */
};

AuxInManager *AuxInManagerCreate(struct DeviceController *controller);
#endif
