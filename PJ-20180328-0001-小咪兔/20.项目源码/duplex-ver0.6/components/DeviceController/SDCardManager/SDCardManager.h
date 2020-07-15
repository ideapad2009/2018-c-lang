#ifndef _SD_CARD_MANAGER_H_
#define _SD_CARD_MANAGER_H_
#include "DeviceManager.h"

struct DeviceController;

typedef struct SDCardManager SDCardManager;

/* Implements DeviceManager */
struct SDCardManager {
    /* extend */
    DeviceManager Based;
    /* private */
};

SDCardManager *SDCardManagerCreate(struct DeviceController *controller);

#endif
