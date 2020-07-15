/**
 * DeviceController interface
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"

#include "DeviceController.h"
#include "DeviceControllerHelper.h"
#include "DeviceCommon.h"
#include "MediaService.h"
#include "DeviceManager.h"
/* only for public constructors */
#include "TouchManager.h"
#include "WifiManager.h"
#include "SDCardManager.h"
#include "AuxInManager.h"
#include "EspAudioAlloc.h"


#define DEVICE_CTRL_TAG                     "DEVICE_CONTROLLER"

/* ================================================
 * DeviceController implementation for wifi manager
 * ================================================
 */
static void WifiSmartConfig(DeviceController *controller)
{
    WifiManager *wifi = (WifiManager *) controller->wifi;
    if (wifi) {
        wifi->smartConfig(wifi);
    } else {
        ESP_LOGE(DEVICE_CTRL_TAG, "WIFI manager not found");
    }
}

static void WifiBleConfig(DeviceController *controller)
{
	WifiManager *wifi = (WifiManager *) controller->wifi;
	if (wifi) {
		wifi->bleConfig(wifi);
	} else {
		ESP_LOGE(DEVICE_CTRL_TAG, "WIFI manager not found");
	}
}

static void WifiBleConfigStop(DeviceController *controller)
{
	WifiManager *wifi = (WifiManager *) controller->wifi;
	if (wifi) {
		wifi->bleConfigStop(wifi);
	} else {
		ESP_LOGE(DEVICE_CTRL_TAG, "WIFI manager not found");
	}
}

//XXX: alternative  --  active one manager a time
static void ControllerActiveManagers(DeviceController *controller)
{
    if (controller->wifi) {
        controller->wifi->active(controller->wifi);
    }
    if (controller->sdcard) {
        controller->sdcard->active(controller->sdcard);
    }
    if (controller->touch) {
        controller->touch->active(controller->touch);
    }
    if (controller->auxIn) {
        controller->auxIn->active(controller->auxIn);
    }

}

static void ControllerEnableWifi(DeviceController *controller)
{
    controller->wifi = (DeviceManager *) WifiConfigCreate(controller);
}

static void ControllerEnableTouch(DeviceController *controller)
{
    controller->touch = (DeviceManager *) TouchManagerCreate(controller);
}

static void ControllerEnableSDcard(DeviceController *controller)
{
    controller->sdcard = (DeviceManager *) SDCardManagerCreate(controller);
    controller->sdcard->pauseMedia = ControllerPauseMedia;
    controller->sdcard->resumeMedia = ControllerResumeMedia;
}

static void ControllerEnableAuxIn(DeviceController *controller)
{
    controller->auxIn = (DeviceManager *) AuxInManagerCreate(controller);  //TODO
    controller->auxIn->pauseMedia = ControllerPauseMedia;
}

DeviceController *DeviceCtrlCreate(struct MediaControl *ctrl)
{
    ESP_LOGI(DEVICE_CTRL_TAG, "Device Controller Create\r\n");
    DeviceController *deviceCtrl = (DeviceController *) EspAudioAlloc(1, sizeof(DeviceController));
    ESP_ERROR_CHECK(!deviceCtrl);
    deviceCtrl->instance = (void *) ctrl;
    deviceCtrl->enableWifi = ControllerEnableWifi;
    deviceCtrl->enableTouch = ControllerEnableTouch;
    deviceCtrl->enableSDcard = ControllerEnableSDcard;
    deviceCtrl->enableAuxIn = ControllerEnableAuxIn;
    deviceCtrl->activeManagers = ControllerActiveManagers;
    //deviceCtrl->wifiSmartConfig = WifiSmartConfig;
    deviceCtrl->wifiBleConfig = WifiBleConfig;
    deviceCtrl->wifiBleConfigStop = WifiBleConfigStop;
    return deviceCtrl;
}
