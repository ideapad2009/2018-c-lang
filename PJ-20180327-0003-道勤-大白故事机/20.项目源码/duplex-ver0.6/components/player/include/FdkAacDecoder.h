#ifndef _FDK_AAC_H_
#define _FDK_AAC_H_

#include "SoftCodec.h"
#include "aacdecoder_lib.h"

typedef struct {
    HANDLE_AACDECODER handle;
    int input_size;
    char *input_buffer;
    int output_size;
    char *output_buffer;
    int _run;
    int _sample_rate;
    int _channels;
    int _bit_rate;
    int frameCount;
} FdkAacCodec;

int FdkAacOpen(SoftCodec *softCodec);
int FdkAacClose(SoftCodec *softCodec);
int FdkAacProcess(SoftCodec *softCodec);
int FdkAacTriggerStop(SoftCodec *softCodec);
#define DECODER_MAX_CHANNELS     2
#define DECODER_BUFFSIZE        (2048 * sizeof(INT_PCM))
#define AAC_BUFFER_SIZE         (2048)
#endif
