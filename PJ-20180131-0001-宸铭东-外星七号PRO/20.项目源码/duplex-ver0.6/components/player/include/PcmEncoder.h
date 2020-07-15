#ifndef _PCM_ENC_CODEC_H_
#define _PCM_ENC_CODEC_H_
#include "SoftCodec.h"

#define PCM_ENC_PACKET_LEN (2048)

typedef struct {
    unsigned char *buffer;
    int _run;
} PCMEncCodec;

int PCMEncOpen(SoftCodec *softCodec);
int PCMEncClose(SoftCodec *softCodec);
int PCMEncProcess(SoftCodec *softCodec);

#endif
