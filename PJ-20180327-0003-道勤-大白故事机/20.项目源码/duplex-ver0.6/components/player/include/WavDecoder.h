#ifndef _WAV_CODEC_H_
#define _WAV_CODEC_H_
#include "SoftCodec.h"

#define WAV_PACKET_LEN (1024)

typedef struct {
    unsigned char *buffer;
    int _run;
    int currentPosition;
    int wavSize;            //data chunk
} WAVCodec;

int WAVOpen(SoftCodec *softCodec);
int WAVClose(SoftCodec *softCodec);
int WAVProcess(SoftCodec *softCodec);
#endif
