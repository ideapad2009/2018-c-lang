#ifndef _M4A_DECODER_H_
#define _M4A_DECODER_H_

#include "SoftCodec.h"
#include "aacdecoder_lib.h"

#define MP4_PARSE_ERROR 0
#define MP4_PARSE_OK 1

typedef struct {
    HANDLE_AACDECODER handle;
    int input_size;
    char *input_buffer;
    int output_size;
    char *output_buffer;
    int _run;
    int _sample_rate;
    int _channels;
    int sampleId;
    int mp4Format;
    int numSamples;
    void *user_data;
    void *audioInfo;
    int offsetIdx;
} M4aCodec;

int M4aOpen(SoftCodec *softCodec);
int M4aClose(SoftCodec *softCodec);
int M4aProcess(SoftCodec *softCodec);
int M4aTriggerStop(SoftCodec *softCodec);
#define DECODER_MAX_CHANNELS     2
#define DECODER_BUFFSIZE        (2048 * sizeof(INT_PCM))
#define AAC_BUFFER_SIZE         (2048)
#endif
