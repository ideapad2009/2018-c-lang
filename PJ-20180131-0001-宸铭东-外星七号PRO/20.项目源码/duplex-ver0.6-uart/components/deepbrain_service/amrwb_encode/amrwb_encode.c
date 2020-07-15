#include <string.h>
#include "esp_log.h"
#include <stdlib.h>
#include <stdio.h>

#include "SoftCodec.h"
#include "FatFsSal.h"
#include "EspAudioAlloc.h"
#include "driver/i2s.h"
#include "amrwb_encode.h"
#include "cmnMemory.h"
#include "device_api.h"

#define AMRWB_ENC_TAG "amrwb encode"

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

void *amrwb_encode_create(VOAMRWBMODE mode)
{
    int ret = 0;

    AMRWB_ENCODE_HANDLE_T *codec = esp32_malloc(sizeof(AMRWB_ENCODE_HANDLE_T));
    if (codec == NULL) 
	{
        ESP_LOGE(AMRWB_ENC_TAG, "calloc failed(line %d)", __LINE__);
        return NULL;
    }

    codec->AudioAPI = esp32_malloc(sizeof(VO_AUDIO_CODECAPI));
    if(codec->AudioAPI == NULL) 
	{
        printf("failed to allocate AudioAPI\n");
        return NULL;
    }
    codec->moper = esp32_malloc(sizeof(VO_MEM_OPERATOR));
    if (codec->moper == NULL) 
	{
        printf("failed to allocate moper\n");
        return NULL;
    }
    codec->useData = esp32_malloc(sizeof(VO_CODEC_INIT_USERDATA));
    if (codec->useData == NULL) 
	{
        printf("failed to allocate useData\n");
        return NULL;
    }
    codec->inData = esp32_malloc(sizeof(VO_CODECBUFFER));
    if (codec->inData == NULL) 
	{
        printf("failed to allocate inData\n");
        return NULL;
    }
    codec->outData = esp32_malloc(sizeof(VO_CODECBUFFER));
    if (codec->outData == NULL) 
	{
        printf("failed to allocate outData\n");
        return NULL;
    }
    codec->outFormat = esp32_malloc(sizeof(VO_AUDIO_OUTPUTINFO));
    if (codec->outFormat == NULL) 
	{
        printf("failed to allocate outFormat\n");
        return NULL;
    }

    codec->mode = mode;
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
    if (ret) 
	{
        printf("get APIs error......");
        return NULL;
    }

    //#######################################   Init Encoding Section   #########################################
    ret = codec->AudioAPI->Init(&codec->hCodec, VO_AUDIO_CodingAMRWB, codec->useData);
    if (ret) {
        printf("APIs init error......");
        return NULL;
    }

    //###################################### set encode Mode ##################################################
    ret = codec->AudioAPI->SetParam(codec->hCodec, VO_PID_AMRWB_FRAMETYPE, &codec->frameType);
    ret = codec->AudioAPI->SetParam(codec->hCodec, VO_PID_AMRWB_MODE, &codec->mode);
    ret = codec->AudioAPI->SetParam(codec->hCodec, VO_PID_AMRWB_DTX, &codec->allow_dtx);
	
    return codec;
}

int amrwb_encode_delete(void *handle)
{
    AMRWB_ENCODE_HANDLE_T *codec = handle;
    if (codec == NULL)
    {
        return CODEC_OK;
    }
    printf("pcmcnt = %d : framecnt = %d : total_bytes = %d\n",codec->pcmcnt, codec->framecnt, codec->total_bytes);

    codec->AudioAPI->Uninit(codec->hCodec);
    if (codec->AudioAPI)
    {
        esp32_free(codec->AudioAPI);
    }
	
    if (codec->moper)
    {
        esp32_free(codec->moper);
    }
	
    if (codec->useData)
    {
        esp32_free(codec->useData);
    }
	
    if (codec->inData)
	{
        esp32_free(codec->inData);
    }
	
    if (codec->outData)
    {
		esp32_free(codec->outData);
    }
	
    if (codec->outFormat)
    {
        esp32_free(codec->outFormat);
    }
	
    if (codec)
    {
        esp32_free(codec);
    }
	
    return CODEC_OK;
}

int amrwb_encode_process(
	void *handle, const unsigned char* pcm_data, size_t pcm_len, unsigned char* out)
{
	int ret = 0;
    AMRWB_ENCODE_HANDLE_T *codec = handle;
    if (codec == NULL)
    {
        return CODEC_FAIL;
    }
	
    codec->inData->Buffer = pcm_data;
    codec->inData->Length = pcm_len;
    codec->outData->Buffer = out;
    
    /* decode one amr block */
    ret = codec->AudioAPI->SetInputData(codec->hCodec, codec->inData);

    do 
	{
        ret = codec->AudioAPI->GetOutputData(codec->hCodec, codec->outData, codec->outFormat);
        if (ret == 0) 
		{
            codec->framecnt++;
            codec->pcmcnt += codec->Relens / 2;
            printf(" Frames processed: %hd\r", codec->framecnt);
            codec->total_bytes += codec->outData->Length;
        }
        else if (ret == VO_ERR_LICENSE_ERROR) 
		{
            printf("Encoder time reach upper limit......");
            fprintf(stderr, "error: %d\n", ret);
            return CODEC_FAIL;
        }
    } while (ret != VO_ERR_INPUT_BUFFER_SMALL);

    return codec->outData->Length;
}

