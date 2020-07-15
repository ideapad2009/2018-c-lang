/**
 * DeviceControllerHelper.c
 * Put all the functions that need access to MediaControl here. All the other source file
 * would better not include any MediaControl related source files. This makes
 * dependencies clear.
 */

#include "DeviceControllerHelper.h"
#include "DeviceController.h"
#include "DeviceManager.h"
#include "MediaControl.h"

void ControllerPauseMedia(DeviceManager *manager)
{
    DeviceController *controller = manager->controller;
    MediaControl *ctrl = (MediaControl *) controller->instance;
    ctrl->_pause(ctrl);
}

void ControllerResumeMedia(DeviceManager *manager)
{
    DeviceController *controller = manager->controller;
    MediaControl *ctrl = (MediaControl *) controller->instance;
    ctrl->_resume(ctrl);
}

