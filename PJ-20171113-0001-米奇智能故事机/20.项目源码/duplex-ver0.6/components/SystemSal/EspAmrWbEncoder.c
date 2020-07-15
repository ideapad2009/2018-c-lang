#include <string.h>
#include "esp_log.h"
#include <stdlib.h>
#include <stdio.h>

#include "SoftCodec.h"
#include "FatFsSal.h"
#include "EspAudioAlloc.h"
#include "driver/i2s.h"

#include "EspAmrWbEncoder.h"
#include "cmnMemory.h"

#define AMRWB_ENC_TAG "SOFTCODEC_ENC_AMRWB"
#define AMRWB_ENC_MEMORY_ANALYSIS

#define VOAMRWB_RFC3267_HEADER_INFO "#!AMR-WB\n"
#define AMRWB_ENC_IN_BUFFER_SZ  640
#define AMRWB_ENC_OUT_BUFFER_SZ  1024

#define DEBUG_AMR_ENC_ISSUE

#ifdef DEBUG_AMR_ENC_ISSUE
static FILE *outAmr = NULL;
#endif

static void i2sMono(void *sBuff, uint32_t len)
{
    int16_t *tempBuf = sBuff;
    int16_t tempBox;
    for (int i = 0; i < len / 2; i += 2) {
        tempBox = tempBuf[i];
        tempBuf[i] = tempBuf[i + 1];
        tempBuf[i + 1] = tempBox;
    }
}


EspAmrWbEncCodec *EspAmrWbEncOpen()
{
    int ret = 0;
    printf("---------EspAmrWb Encoding----------\n");
#ifdef AMRWB_ENC_MEM_IN_PSRAM
    EspAmrWbEncCodec *codec = EspAudioAlloc(1, sizeof(EspAmrWbEncCodec));
#else
    EspAmrWbEncCodec *codec = calloc(1, sizeof(EspAmrWbEncCodec));
#endif
    if (codec == NULL) {
        ESP_LOGE(AMRWB_ENC_TAG, "calloc failed(line %d)", __LINE__);
        return NULL;
    }

#ifdef AMRWB_ENC_MEM_IN_PSRAM
    codec->inBuf = EspAudioAlloc(1, AMRWB_ENC_IN_BUFFER_SZ);
#else
    codec->inBuf = calloc(1, AMRWB_ENC_IN_BUFFER_SZ);
#endif
    if (codec->inBuf == NULL) {
        ESP_LOGE(AMRWB_ENC_TAG, "calloc failed(line %d)", __LINE__);
        return NULL;
    }
#ifdef AMRWB_ENC_MEM_IN_PSRAM
    codec->outBuf = EspAudioAlloc(1, AMRWB_ENC_OUT_BUFFER_SZ);
#else
    codec->outBuf = calloc(1, AMRWB_ENC_OUT_BUFFER_SZ);
#endif
    if (codec->outBuf == NULL) {
        ESP_LOGE(AMRWB_ENC_TAG, "calloc failed(line %d)", __LINE__);
        return NULL;
    }
#ifdef DEBUG_AMR_ENC_ISSUE
    char fileName[20] = {'//','s','d','c','a','r','d','//', 'z', 'g', 'c','.','a','m','r','\0'};
    printf("fileName = %s\n",fileName);
    outAmr = fopen(fileName, "wb+");
    if (!outAmr) {
        perror(fileName);
        return NULL;
    }
#endif /* DEBUG_AMR_ENC_ISSUE */

    codec->AudioAPI = malloc(sizeof(VO_AUDIO_CODECAPI));
    if(codec->AudioAPI == NULL) {
        printf("failed to allocate AudioAPI\n");
        return NULL;
    }
    codec->moper = malloc(sizeof(VO_MEM_OPERATOR));
    if (codec->moper == NULL) {
        printf("failed to allocate moper\n");
        return NULL;
    }
    codec->useData = malloc(sizeof(VO_CODEC_INIT_USERDATA));
    if (codec->useData == NULL) {
        printf("failed to allocate useData\n");
        return NULL;
    }
    codec->inData = malloc(sizeof(VO_CODECBUFFER));
    if (codec->inData == NULL) {
        printf("failed to allocate inData\n");
        return NULL;
    }
    codec->outData = malloc(sizeof(VO_CODECBUFFER));
    if (codec->outData == NULL) {
        printf("failed to allocate outData\n");
        return NULL;
    }
    codec->outFormat = malloc(sizeof(VO_AUDIO_OUTPUTINFO));
    if (codec->outFormat == NULL) {
        printf("failed to allocate outFormat\n");
        return NULL;
    }

    codec->mode = VOAMRWB_MD2385;
    codec->allow_dtx = 0;
    codec->frameType = VOAMRWB_RFC3267;
    codec->hCodec = NULL;

    codec->moper->Alloc = cmnMemAlloc;
    codec->moper->Copy = cmnMemCopy;
    codec->moper->Free = cmnMemFree;
    codec->moper->Set = cmnMemSet;
    codec->moper->Check = cmnMemCheck;

    codec->useData->memflag = VO_IMF_USERMEMOPERATOR;
    codec->useData->memData = (VO_PTR)(codec->moper);

    ret = voGetAMRWBEncAPI(codec->AudioAPI);
    if (ret) {
        printf("get APIs error......");
        return NULL;
    }

    //#######################################   Init Encoding Section   #########################################
#ifdef AMRWB_ENC_MEMORY_ANALYSIS
    EspAudioPrintMemory(AMRWB_ENC_TAG);
#endif
    ret = codec->AudioAPI->Init(&codec->hCodec, VO_AUDIO_CodingAMRWB, codec->useData);
    if (ret) {
        printf("APIs init error......");
        return NULL;
    }
#ifdef AMRWB_ENC_MEMORY_ANALYSIS
    EspAudioPrintMemory(AMRWB_ENC_TAG);
#endif
    i2s_set_clk((i2s_port_t)0, 16000, 16, 1);
    codec->Relens = i2s_read_bytes(0, (char *)(codec->inBuf), AMRWB_ENC_IN_BUFFER_SZ, portMAX_DELAY);
    if (codec->Relens < AMRWB_ENC_IN_BUFFER_SZ) {
        printf("get pcm data error......");
        return NULL;
    }
    i2sMono((codec->inBuf), AMRWB_ENC_IN_BUFFER_SZ);

    //###################################### set encode Mode ##################################################
    ret = codec->AudioAPI->SetParam(codec->hCodec, VO_PID_AMRWB_FRAMETYPE, &codec->frameType);
    ret = codec->AudioAPI->SetParam(codec->hCodec, VO_PID_AMRWB_MODE, &codec->mode);
    ret = codec->AudioAPI->SetParam(codec->hCodec, VO_PID_AMRWB_DTX, &codec->allow_dtx);

    int size = 0;
    if (codec->frameType == VOAMRWB_RFC3267)
    {
        /* write RFC3267 Header info to indicate single channel AMR file storage format */
        size = (int)strlen(VOAMRWB_RFC3267_HEADER_INFO);
        memcpy(codec->outBuf, VOAMRWB_RFC3267_HEADER_INFO, size);
    }

#ifdef DEBUG_AMR_ENC_ISSUE
    if(size)
        FatfsComboWrite(codec->outBuf, 1, size, outAmr);
#else

#endif /* DEBUG_AMR_ENC_ISSUE */
        //string_printf(codec->outBuf, size);
    codec->eofFile = 0;
    codec->pcmcnt = codec->Relens / 2;
    codec->framecnt = 0;
    codec->total_bytes = 9;
    return codec;
}

int EspAmrWbEncClose(EspAmrWbEncCodec *softCodec)
{
    EspAmrWbEncCodec *codec = softCodec;
    if (codec == NULL)
        return CODEC_OK;

    printf("pcmcnt = %d : framecnt = %d : total_bytes = %d\n",codec->pcmcnt, codec->framecnt, codec->total_bytes);

    codec->AudioAPI->Uninit(codec->hCodec);
    if (codec->AudioAPI)
        free(codec->AudioAPI);
    if (codec->moper)
        free(codec->moper);
    if (codec->useData)
        free(codec->useData);
    if (codec->inData)
        free(codec->inData);
    if (codec->outData)
        free(codec->outData);
    if (codec->outFormat)
        free(codec->outFormat);
    if (codec->inBuf)
        free(codec->inBuf);
    if (codec->outBuf)
        free(codec->outBuf);
    if (codec)
        free(codec);
#ifdef DEBUG_AMR_ENC_ISSUE
    fclose(outAmr);
#endif

    return CODEC_OK;
}

int EspAmrWbEncProcess(EspAmrWbEncCodec *softCodec)
{
    EspAmrWbEncCodec *codec = softCodec;
    if (codec == NULL)
        return CODEC_FAIL;
    codec->inData->Buffer = (unsigned char *)codec->inBuf;
    codec->inData->Length = codec->Relens;
    codec->outData->Buffer = codec->outBuf;

    int ret = 0, bread;
        /* decode one amr block */
    ret = codec->AudioAPI->SetInputData(codec->hCodec, codec->inData);

    do {
        ret = codec->AudioAPI->GetOutputData(codec->hCodec, codec->outData, codec->outFormat);
        if (ret == 0) {
            codec->framecnt++;
            codec->pcmcnt += codec->Relens / 2;
            printf(" Frames processed: %hd\r", codec->framecnt);
            codec->total_bytes += codec->outData->Length;
#ifdef DEBUG_AMR_ENC_ISSUE
            FatfsComboWrite(codec->outData->Buffer, 1, codec->outData->Length, outAmr);
            //string_printf(codec->outData->Buffer, codec->outData->Length);
#else

#endif
        }
        else if (ret == VO_ERR_LICENSE_ERROR) {
            printf("Encoder time reach upper limit......");
            fprintf(stderr, "error: %d\n", ret);
            return CODEC_FAIL;
        }
    } while (ret != VO_ERR_INPUT_BUFFER_SMALL);

    if (!codec->eofFile) {
        codec->Relens = i2s_read_bytes(0, (char *)(codec->inBuf), AMRWB_ENC_IN_BUFFER_SZ, portMAX_DELAY);
        if(codec->Relens < AMRWB_ENC_IN_BUFFER_SZ) {
            codec->eofFile = 1;
            ESP_LOGE(AMRWB_ENC_TAG, "---------------Encoding finish--------------");
            return CODEC_DONE;
        }
        i2sMono((codec->inBuf), AMRWB_ENC_IN_BUFFER_SZ);
    }
    return CODEC_OK;
}

