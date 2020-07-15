#ifndef _AMR_CODEC_H_
#define _AMR_CODEC_H_
#include "SoftCodec.h"

int AmrDecOpen(SoftCodec *softCodec);
int AmrDecClose(SoftCodec *softCodec);
int AmrDecProcess(SoftCodec *softCodec);

#endif
