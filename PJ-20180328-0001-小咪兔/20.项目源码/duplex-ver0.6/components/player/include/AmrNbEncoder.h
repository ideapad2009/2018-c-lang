#ifndef _AMRNB_ENCCODER_H_
#define _AMRNB_ENCCODER_H_
#include "SoftCodec.h"

typedef struct  AmrNbEncCodec {
    void *encoder;
    uint8_t *inBuf;
    uint8_t *outBuf;

    int amrnbEncPcmNum;
    int amrnbEncFrameNum;
} AmrNbEncCodec;

int AmrNbEncOpen(SoftCodec *softCodec);
int AmrNbEncClose(SoftCodec *softCodec);
int AmrNbEncProcess(SoftCodec *softCodec);
#endif
