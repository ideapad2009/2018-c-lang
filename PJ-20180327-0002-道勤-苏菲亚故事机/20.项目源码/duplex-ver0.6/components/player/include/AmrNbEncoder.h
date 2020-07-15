#ifndef _AMRNB_ENCCODER_H_
#define _AMRNB_ENCCODER_H_
#include "SoftCodec.h"

int AmrNbEncOpen(SoftCodec *softCodec);
int AmrNbEncClose(SoftCodec *softCodec);
int AmrNbEncProcess(SoftCodec *softCodec);
int AmrNbEncTriggerStop(SoftCodec *softCodec);
#endif
