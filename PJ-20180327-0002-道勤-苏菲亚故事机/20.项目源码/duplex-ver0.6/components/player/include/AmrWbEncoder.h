#ifndef _AMRWB_ENCCODER_H_
#define _AMRWB_ENCCODER_H_
#include "SoftCodec.h"

int AmrWbEncOpen(SoftCodec *softCodec);
int AmrWbEncClose(SoftCodec *softCodec);
int AmrWbEncProcess(SoftCodec *softCodec);
int AmrWbEncTriggerStop(SoftCodec *softCodec);
#endif
