#ifndef _ESP_AMRWB_ENCCODER_H_
#define _ESP_AMRWB_ENCCODER_H_
#include "voAMRWB.h"

typedef struct  EspAmrWbEncCodec {
    unsigned char *inBuf;
    unsigned char *outBuf;

    int mode;
    int allow_dtx;
    VOAMRWBFRAMETYPE frameType;
    int eofFile;
    int Relens;

    VO_AUDIO_CODECAPI       *AudioAPI;
    VO_MEM_OPERATOR         *moper;
    VO_CODEC_INIT_USERDATA  *useData;
    VO_HANDLE               hCodec;
    VO_CODECBUFFER          *inData;
    VO_CODECBUFFER          *outData;
    VO_AUDIO_OUTPUTINFO     *outFormat;

    int pcmcnt;
    int framecnt;
    int total_bytes;
} EspAmrWbEncCodec;

EspAmrWbEncCodec *EspAmrWbEncOpen();
int EspAmrWbEncClose(EspAmrWbEncCodec *softCodec);
int EspAmrWbEncProcess(EspAmrWbEncCodec *softCodec);
#endif
