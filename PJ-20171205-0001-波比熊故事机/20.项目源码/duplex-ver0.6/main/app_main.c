#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "tcpip_adapter.h"

#include "FdkAacDecoder.h"
#include "MadMp3Decoder.h"
// #include "HelixMp3Decoder.h"
#include "StageFrightMp3Decoder.h"
// #include "FluendoMp3Decoder.h"
#include "OggDecoder.h"
#include "FlacDecoder.h"
#include "WavDecoder.h"
#include "OpusDecoder.h"
#include "PcmDecoder.h"
#include "M4aDecoder.h"
#include "AmrNbEncoder.h"
#include "AmrNbDecoder.h"

#include "OpusEncoder.h"
#include "PcmEncoder.h"

#include "MediaControl.h"
#include "DeviceController.h"
#include "TerminalControlService.h"
#include "TouchControlService.h"
#include "led_ctrl.h"
#include "userconfig.h"
#include "EspAudio.h"
#include "InterruptionSal.h"
#include "deepbrain_service.h"
#include "flash_config_manage.h"
#include "player_middleware.h"
#include "mpush_service.h"
#include "mcu_serial_comm.h"
#include "collection_function.h"
#include "guo_xue_function_manage.h"
#include "bedtime_story_key_manage.h"
#include "toneuri.h"

#if MODULE_LOW_POWER_MANAGE
#include "low_power_manage.h"
#endif

#if MODULE_OTA_UPDATE
#include "ota_service.h"
#endif

#if IDF_3_0 && CONFIG_BT_ENABLED
#include "esp_bt.h"
#endif

//select classic BT in menuconfig if want to use BT audio, otherwise Wi-Fi audio is on by default
#ifdef CONFIG_CLASSIC_BT_ENABLED
#include "BluetoothControlService.h"
#else
#include "tcpip_adapter.h"

#if (defined CONFIG_ENABLE_TURINGAIWIFI_SERVICE || defined CONFIG_ENABLE_TURINGWECHAT_SERVICE)
#include "TuringRobotService.h"
#endif
#endif /* CONFIG_CLASSIC_BT_ENABLED */

#define APP_TAG "AUDIO_MAIN"

const PlayerConfig playerConfig = {
    .bufCfg = {
#if (CONFIG_SPIRAM_BOOT_INIT || CONFIG_MEMMAP_SPIRAM_ENABLE_MALLOC)
        .inputBufferLength = 2 * 1024 * 1024, //input buffer size in bytes, this buffer is used to store the data downloaded from http or fetched from TF-Card or MIC
#else
        .inputBufferLength = 20 * 1024, //input buffer size in bytes, this buffer is used to store the data downloaded from http or fetched from TF-Card or MIC
#endif
        .outputBufferLength = 20 * 1024,//output buffer size in bytes, is used to store the data before play or write into TF-Card
#ifndef CONFIG_CLASSIC_BT_ENABLED
        .processBufferLength = 10
#endif
    },
    //comment any of below streams to disable corresponding streams in order to save RAM.
    .inputStreamType = IN_STREAM_I2S    //this is recording stream that records data from mic
    | IN_STREAM_HTTP   //http stream can support fetching data from a website and play it
    | IN_STREAM_LIVE   //supports m3u8 stream playing.
    | IN_STREAM_FLASH  //support flash music playing, refer to [Flash Music] section in "docs/ESP-Audio_Programming_Guide.md" for detail
    | IN_STREAM_SDCARD//support TF-Card playing. Refer to "SDCard***.c" for more information
    | IN_STREAM_ARRAY
    ,
    .outputStreamType = OUT_STREAM_I2S      //output the music via i2s protocol, see i2s configuration in "MediaHal.c"
    | OUT_STREAM_SDCARD  //write the music data into TF-Card, usually for recording
    ,

#ifndef CONFIG_CLASSIC_BT_ENABLED
    .playListEn = ESP_AUDIO_DISABLE,
#else
    .playListEn = ESP_AUDIO_DISABLE,
#endif
    .toneEn = ESP_AUDIO_ENABLE,
    .playMode = MEDIA_PLAY_ONE_SONG,
};

void app_main()
{
    init_device_params();
	UserGpioInit();
	GpioInterInstall();
	tcpip_adapter_init();
    MediaControl* player;
    if (EspAudioInit(&playerConfig, &player) != AUDIO_ERR_NO_ERROR)
        return;
    EspAudioSamplingSetup(48000);

#if MODULE_DEEPBRAIN	
	DeepBrainService *deepBrain = DeepBrainCreate();
    player->addService(player, (MediaService *)deepBrain);
#endif

    /*--------------------------------------------------------------
    |    Any service can be commented out to disable such feature   |
    ---------------------------------------------------------------*/

    /* shell service (uart terminal), refer to "/components/TerminalService" directory*/
    TerminalControlService* term = TerminalControlCreate();
    player->addService(player, (MediaService*) term);

    /*------------------------------------------------------------------------------------
    |    Wi-Fi and TF-Card and Button and Auxiliary are components of DeviceController    |
    |    Any device can be commented out to disable such feature                          |
    -------------------------------------------------------------------------------------*/
    player->deviceController = DeviceCtrlCreate(player);
    DeviceController* deviceController =  player->deviceController;

    /*---------------------------------------------------------
    |    Soft codec libraries supporting corresponding music   |
    |    Any library can be commented out to reduce bin size   |
    ----------------------------------------------------------*/
    /* decoder */
    // EspAudioCodecLibAdd(CODEC_DECODER, 9 * 1024, "mp4", M4aOpen, M4aProcess, M4aClose, M4aTriggerStop);
    EspAudioCodecLibAdd(CODEC_DECODER, 9 * 1024, "m4a", M4aOpen, M4aProcess, M4aClose, M4aTriggerStop);
    // EspAudioCodecLibAdd(CODEC_DECODER, 9 * 1024, "aac", FdkAacOpen, FdkAacProcess, FdkAacClose, FdkAacTriggerStop);
    // EspAudioCodecLibAdd(CODEC_DECODER, 10 * 1024, "ogg", OGGOpen, OGGProcess, OGGClose, OGGTriggerStop);
    // EspAudioCodecLibAdd(CODEC_DECODER, 4 * 1024, "flac", FLACOpen, FLACProcess, FLACClose, FLACTriggerStop);
    // EspAudioCodecLibAdd(CODEC_DECODER, 30 * 1024, "opus", OpusOpen, OpusProcess, OpusClose, NULL);
    EspAudioCodecLibAdd(CODEC_DECODER, 5 * 1024, "amr", AmrNbDecOpen, AmrNbDecProcess, AmrNbDecClose, NULL);
    //EspAudioCodecLibAdd(CODEC_DECODER, 5 * 1024, "mp3", HelixMP3Open, HelixMP3Process, HelixMP3Close, HelixMP3TriggerStop);
    //EspAudioCodecLibAdd(CODEC_DECODER, 10 * 1024, "mp3", MadMP3Open, MadMP3Process, MadMP3Close, MadMP3TriggerStop);
    EspAudioCodecLibAdd(CODEC_DECODER, 10 * 1024, "mp3", StageFrightMP3Open, StageFrightMP3Process, StageFrightMP3Close, StageFrightMP3TriggerStop);
    //EspAudioCodecLibAdd(CODEC_DECODER, 30 * 1024, "mp1", FluendoMP3Open, FluendoMP3Process, FluendoMP3Close, FluendoMP3TriggerStop);
    //EspAudioCodecLibAdd(CODEC_DECODER, 30 * 1024, "mp2", FluendoMP3Open, FluendoMP3Process, FluendoMP3Close, FluendoMP3TriggerStop);
    //EspAudioCodecLibAdd(CODEC_DECODER, 30 * 1024, "mp3", FluendoMP3Open, FluendoMP3Process, FluendoMP3Close, FluendoMP3TriggerStop);
    EspAudioCodecLibAdd(CODEC_DECODER, 3 * 1024, "wav", WAVOpen, WAVProcess, WAVClose, NULL);
    //EspAudioCodecLibAdd(CODEC_DECODER, 3 * 1024, "pcm", PCMDecOpen, PCMDecProcess, PCMDecClose, NULL);
    /* encoder */
    //EspAudioCodecLibAdd(CODEC_ENCODER, 23 * 1024, "opus", OpusEncOpen, OpusEncProcess, OpusEncClose, NULL);
//    EspAudioCodecLibAdd(CODEC_ENCODER, 8 * 1024, "amr", AmrNbEncOpen, AmrNbEncProcess, AmrNbEncClose, NULL);//AMR NB, 8K mono
//    EspAudioCodecLibAdd(CODEC_ENCODER, 15 * 1024, "Wamr", AmrWbEncOpen, AmrWbEncProcess, AmrWbEncClose, AmrWbEncTriggerStop);//AMR WB, 16K mono
//    EspAudioCodecLibAdd(CODEC_ENCODER, 23 * 2* 1024, "pom", OpusEncOpen, OpusEncProcess, OpusEncClose, NULL);
    EspAudioCodecLibAdd(CODEC_ENCODER, 2 * 1024, "pcm", PCMEncOpen, PCMEncProcess, PCMEncClose, NULL);

	create_player_middleware();
#if MODULE_BOBI_CASE
	player_mdware_play_tone(FLASH_MUSIC_B_01_I_AM_BOBY);
//	EspAudioPlay(tone_uri[TONE_TYPE_01_I_AM_BOBY]);

#endif
    // deviceController->enableAuxIn(deviceController);
    deviceController->enableWifi(deviceController);
    deviceController->enableSDcard(deviceController);

#if MODULE_COLLECTION_FUNCTION
	init_collection_function();
#endif

#if MODULE_GUOXUE_FUNCTION
	guo_xue_init();
#endif

	bedtime_story_function_init();

#if MODULE_LOW_POWER_MANAGE
	low_power_manage_init();
#endif

#if MODULE_MPUSH
	MPUSH_SERVICE_T *mpush = mpush_service_create();
	player->addService(player, (MediaService *) mpush);
#endif

		OTA_SERVICE_T *ota = ota_service_create();
		player->addService(player, (MediaService *) ota);

    /* Start all the services and device-controller*/
    player->activeServices(player);
    ESP_LOGI(APP_TAG, "[APP] Waiting command, freemem: %d...\r\n", esp_get_free_heap_size());

//    void PrintTask(void *arg);
//    xTaskCreate(&PrintTask, "PrintTask", 2048, NULL, 1, NULL);
}

void PrintTask(void* arg)
{
#define BUF_SIZE 5 * 1024
    char* tasklist = EspAudioAlloc(1, BUF_SIZE);
    while (1) {
        EspAudioPrintMemory(APP_TAG);
        vTaskDelay(4000 / portTICK_RATE_MS);
    }
    free(tasklist);
}
