#include <string.h>
#include "DeviceManager.h"
#include "DeviceController.h"
#include "DeviceCommon.h"

void InitManager(DeviceManager *manager, DeviceController *controller)
{
    manager->controller = controller;
    manager->instance = controller->instance;
}
