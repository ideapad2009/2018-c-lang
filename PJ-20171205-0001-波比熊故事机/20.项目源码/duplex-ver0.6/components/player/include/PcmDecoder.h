#ifndef _PCM_DEC_CODEC_H_
#define _PCM_DEC_CODEC_H_
#include "SoftCodec.h"

#define PCM_DEC_PACKET_LEN (2048)

typedef struct {
    unsigned char *buffer;
    int _run;
    int currentPosition;
} PCMDecCodec;

int PCMDecOpen(SoftCodec *softCodec);
int PCMDecClose(SoftCodec *softCodec);
int PCMDecProcess(SoftCodec *softCodec);
#endif
