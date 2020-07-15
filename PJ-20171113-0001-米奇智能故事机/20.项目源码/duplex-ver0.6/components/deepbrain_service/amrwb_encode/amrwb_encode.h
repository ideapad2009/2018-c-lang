#ifndef _AMRWB_ENCODE_H_
#define _AMRWB_ENCODE_H_
#include "voAMRWB.h"

#define VOAMRWB_RFC3267_HEADER_INFO "#!AMR-WB\n"
#define AMRWB_ENC_IN_BUFFER_SZ  640
#define AMRWB_ENC_OUT_BUFFER_SZ  (AMRWB_ENC_IN_BUFFER_SZ/10)

typedef struct 
{
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
} AMRWB_ENCODE_HANDLE_T;

void *amrwb_encode_create(VOAMRWBMODE mode);
int amrwb_encode_delete(void *handle);
int amrwb_encode_process(void *handle, const unsigned char* pcm_data, size_t pcm_len, unsigned char* out);

#endif
