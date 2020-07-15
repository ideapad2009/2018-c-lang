#ifndef _OPUS_ENCCODER_H_
#define _OPUS_ENCCODER_H_
#include "SoftCodec.h"
#include "opus.h"
typedef struct {
    OpusEncoder *encoder;
    unsigned char *opus_buffer;
    unsigned char *pcm_buffer;
    int _run;
} OpusEncCodec;

int OpusEncOpen(SoftCodec *softCodec);
int OpusEncClose(SoftCodec *softCodec);
int OpusEncProcess(SoftCodec *softCodec);
#endif
