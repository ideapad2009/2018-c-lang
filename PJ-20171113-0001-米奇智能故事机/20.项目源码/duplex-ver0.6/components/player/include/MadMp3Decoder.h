#ifndef _MAD_MP3_CODEC_H_
#define _MAD_MP3_CODEC_H_
#include "SoftCodec.h"
#include "mad.h"

#define MAD_MP3_BUFFER_SZ (2106)

typedef struct {
    unsigned char *buffer;
    struct mad_decoder *decoder;
    int _run;
    int currentPosition;
    int _skipId3;
    int framecnt;
    int pcmcnt;
} MadMP3Codec;

int MadMP3Open(SoftCodec *softCodec);
int MadMP3Close(SoftCodec *softCodec);
int MadMP3Process(SoftCodec *softCodec);
int MadMP3TriggerStop(SoftCodec *softCodec);
#endif
