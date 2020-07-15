#ifndef _FLAC_CODEC_H_
#define _FLAC_CODEC_H_
#include "SoftCodec.h"
#include "FLAC/stream_decoder.h"
#include "share/compat.h"

typedef struct {
    FLAC__StreamDecoder *decoder;
    unsigned char *buffer;
    int _run;
    int currentPosition;
} FLACCodec;

int FLACOpen(SoftCodec *softCodec);
int FLACClose(SoftCodec *softCodec);
int FLACProcess(SoftCodec *softCodec);
int FLACTriggerStop(SoftCodec *softCodec);
#endif
