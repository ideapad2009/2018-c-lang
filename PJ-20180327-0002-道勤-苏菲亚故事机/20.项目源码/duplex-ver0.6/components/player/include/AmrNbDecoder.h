#ifndef _AMRNB_CODEC_H_
#define _AMRNB_CODEC_H_
#include "SoftCodec.h"

typedef struct AmrNbDecCodec{
    void *handle;
    uint8_t *inBuf;
    int16_t *outBuf;

    int amrnbDecFrameNum;
    int amrnbDecPcmNum;
} AmrNbDecCodec;

int AmrNbDecOpen(SoftCodec *softCodec);
int AmrNbDecClose(SoftCodec *softCodec);
int AmrNbDecProcess(SoftCodec *softCodec);
#endif
