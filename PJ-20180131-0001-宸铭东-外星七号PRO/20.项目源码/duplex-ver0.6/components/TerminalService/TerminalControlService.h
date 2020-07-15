#ifndef _TERMINAL_CTRL_SERVICE_H_
#define _TERMINAL_CTRL_SERVICE_H_
#include "MediaService.h"


typedef struct TerminalControlService //extern from TreeUtility
{
    /*relation*/
    MediaService Based;
} TerminalControlService;

TerminalControlService *TerminalControlCreate();
#endif
