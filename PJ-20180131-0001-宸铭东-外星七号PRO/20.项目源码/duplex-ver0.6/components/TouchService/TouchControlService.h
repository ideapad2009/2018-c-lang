#ifndef _TOUCH_CONTROL_SERVICE_H_
#define _TOUCH_CONTROL_SERVICE_H_
#include "MediaService.h"

typedef struct TouchControlService { //extern from TreeUtility
    /*relation*/
    MediaService Based;
    int _smartCfg;
    int _bleCfg;
    /*private*/
} TouchControlService;

TouchControlService* TouchControlCreate();

#endif
