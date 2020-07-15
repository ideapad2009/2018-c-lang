#include <string.h>
#include "NotifyHelper.h"
#include "DeviceController.h"
#include "MediaControl.h"
#include "MediaService.h"
#include "PlaylistManager.h"
#include "DeviceCommon.h"

#define NOTIFY_TAG  "EVENT_NOTIFY"

void SDCardEvtNotify(DeviceController *controller, DeviceNotifyType type, void *data, int len)
{
    MediaControl *ctrl = (MediaControl *) controller->instance;
    // AudioStream *inputStream = ctrl->inputStream;
    // AudioStream *outputStream = ctrl->outputStream;
    DeviceNotification note;
    memset(&note, 0x00, sizeof(DeviceNotification));
    note.type = type;
    //reserve note.targetService to be set at controller->notifyServices
    note.data = data;
    note.len = len;

    PlaylistManager *playlistManager = ctrl->playlistManager;
    if(playlistManager){
        note.receiver = (PlaylistManager *) playlistManager;
        playlistManager->deviceEvtNotified(&note);
    }
}

static void NotifyServices(DeviceController *controller, DeviceNotifyType type, void *data, int len)
{
    MediaControl *ctrl = (MediaControl *) controller->instance;
    MediaService *service = ctrl->services;
    DeviceNotification note;
    memset(&note, 0x00, sizeof(DeviceNotification));
    note.type = type;
    note.data = data;
    note.len = len;
    while (service->next != NULL) {
        service = service->next;
        note.receiver = (void *) service;
        if (service->deviceEvtNotified) {
            service->deviceEvtNotified(&note);
        }
    }
}

void TouchpadEvtNotify(DeviceController *controller, DeviceNotifyType type, void *data, int len)
{
    NotifyServices(controller, type, data, len);
}

void WifiEvtNotify(DeviceController *controller, DeviceNotifyType type, void *data, int len)
{
    NotifyServices(controller, type, data, len);
}

void AuxInEvtNotify(DeviceController *controller, DeviceNotifyType type, void *data, int len)
{
    MediaControl *ctrl = (MediaControl *) controller->instance;
    if (*((DeviceNotifyAuxInMsg *) data) == DEVICE_NOTIFY_AUXIN_INSERTED) {
        ctrl->audioInfo.player.status = AUDIO_STATUS_AUX_IN;
    } else if (*((DeviceNotifyAuxInMsg *) data) == DEVICE_NOTIFY_AUXIN_REMOVED) {
        ctrl->audioInfo.player.status = AUDIO_STATUS_PAUSED;
    }
    NotifyServices(controller, type, data, len);
}
