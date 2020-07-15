#ifndef _OGG_CODEC_H_
#define _OGG_CODEC_H_
#include "SoftCodec.h"
#include "ivorbiscodec.h"
#include "ivorbisfile.h"

#define OGG_BUFFER_SZ (4096)

typedef struct {
    OggVorbis_File *vf;
    unsigned char *buffer;
    int _run;
    int currentPosition;

    int oggframecnt;
    int oggpcmcnt;
} OGGCodec;

int OGGOpen(SoftCodec *softCodec);
int OGGClose(SoftCodec *softCodec);
int OGGProcess(SoftCodec *softCodec);
int OGGTriggerStop(SoftCodec *softCodec);
#endif
