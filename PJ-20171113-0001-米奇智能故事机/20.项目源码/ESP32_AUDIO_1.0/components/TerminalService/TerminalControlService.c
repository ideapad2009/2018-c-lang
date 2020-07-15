#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_deep_sleep.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "soc/io_mux_reg.h"

#include "esp_shell.h"
#include "TerminalControlService.h"
#include "MediaHal.h"
#include "toneuri.h"
#include "ES8388_interface.h"
#include "EspAudioAlloc.h"
#include "userconfig.h"
#include "EspAudio.h"
#include "flash_config_manage.h"
#include "memo_service.h"
#include "asr_service.h"
#ifdef CONFIG_CLASSIC_BT_ENABLED
#include "esp_a2dp_api.h"
#endif

#define TERM_TAG                            "TERM_CTRL"
#define TERM_SERV_TASK_PRIORITY             5
#define TERM_SERV_TASK_STACK_SIZE           2048

xQueueHandle xQuePlayerStatus;

void playerStatusUpdatedToTerm(ServiceEvent *evt)
{
}


/*---------------------
|    System Commands   |
----------------------*/

#include "soc/rtc_cntl_reg.h"
// need to 0xff600000
void RTC_IRAM_ATTR esp_wake_deep_sleep(void)
{
    esp_default_wake_deep_sleep();
    gpio_pad_unhold(16);
    REG_SET_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_XPD_SDIO_REG);
    REG_SET_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_FORCE);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFH_SDIO, 0x3, RTC_CNTL_DREFH_SDIO_S);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFM_SDIO, 0x3, RTC_CNTL_DREFM_SDIO_S);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFL_SDIO, 0x3, RTC_CNTL_DREFL_SDIO_S);
    REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_TIEH);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_PD_EN, 1, RTC_CNTL_SDIO_PD_EN_S);
}

#include "rom/gpio.h"
IRAM_ATTR void terSleep(void *ref, int argc, char *argv[])
{
    int mode;
    if (argc == 1) {
        char num[10] = "";
        strncpy(num, argv[0], 9);
        mode = atoi(num);
    } else {
        ESP_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    ESP_LOGI(TERM_TAG, "Ready sleep,mode:%d", mode);
    const int ext_wakeup_pin_1 = 36;
    const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
    if (mode) {
        esp_deep_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ALL_LOW);
    } else {
        esp_wifi_stop();
        esp_deep_sleep_enable_ext0_wakeup(ext_wakeup_pin_1, ESP_EXT1_WAKEUP_ALL_LOW);
    }


    REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_XPD_SDIO_REG);
    REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_FORCE);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFH_SDIO, 0x0, RTC_CNTL_DREFH_SDIO_S);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFM_SDIO, 0x0, RTC_CNTL_DREFM_SDIO_S);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFL_SDIO, 0x1, RTC_CNTL_DREFL_SDIO_S);
    REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_TIEH);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_PD_EN, 1, RTC_CNTL_SDIO_PD_EN_S);

#if 0 // RTC pins
    gpio_pulldown_dis(36);
    gpio_pullup_dis(36);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_36);

    gpio_pulldown_dis(37);
    gpio_pullup_dis(37);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_37);

    gpio_pulldown_dis(35);
    gpio_pullup_dis(35);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_35);

    gpio_pulldown_dis(34);
    gpio_pullup_dis(34);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_34);

    gpio_pulldown_dis(33);
    gpio_pullup_dis(33);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_33);

    gpio_pulldown_dis(32);
    gpio_pullup_dis(32);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_32);

    gpio_pulldown_dis(25);
    gpio_pullup_dis(25);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_25);

    gpio_pulldown_dis(26);
    gpio_pullup_dis(26);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_26);

    gpio_pulldown_dis(27);
    gpio_pullup_dis(27);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_27);

#endif


#if 0 // Disable SD card pins
    gpio_pulldown_dis(12);
    gpio_pullup_dis(12);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_12);

    gpio_pulldown_dis(13);
    gpio_pullup_dis(13);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_13);

    gpio_pulldown_dis(14);
    gpio_pullup_dis(14);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_14);

    gpio_pulldown_dis(15);
    gpio_pullup_dis(15);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_15);

    gpio_pulldown_dis(2);
    gpio_pullup_dis(2);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_2);

    gpio_pulldown_dis(0);
    gpio_pullup_dis(0);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_0);

    gpio_pulldown_dis(4);
    gpio_pullup_dis(4);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_4);

#endif

    esp_deep_sleep_start();
}

static void reboot(void *ref, int argc, char *argv[])
{
    esp_restart();
}

static void SwitchBin(void *ref, int argc, char *argv[])
{
	/*
    if (SwitchBootPartition() == ESP_OK) {
#ifndef CONFIG_CLASSIC_BT_ENABLED
        if (esp_wifi_stop() != 0) {
            ESP_LOGE(TERM_TAG, "esp_wifi_stop failed");
            SwitchBootPartition();
            return;
        }
#endif
#ifdef CONFIG_CLASSIC_BT_ENABLED
        if (esp_a2d_sink_disconnect(NULL) != 0) {
            ESP_LOGE(TERM_TAG, "esp_a2d_sink_disconnect failed");
            SwitchBootPartition();
            return;
        }
#endif
        MediaHalPaPwr(0);
        esp_restart();
    }
    */
}

static void getMem(void *ref, int argc, char *argv[])
{
    EspAudioPrintMemory(TERM_TAG);
}

extern void PlayerBackupInfoPrint();
static void backupinfoget(void *ref, int argc, char *argv[])
{
    PlayerBackupInfoPrint();
}




static void tasklist(void *ref, int argc, char *argv[])
{
#define BUF_SIZE 5 * 1024
    char *tasklist = EspAudioAlloc(1, BUF_SIZE);
    if (tasklist) {
#if CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
        // vTaskList(tasklist);
        ESP_LOGI(TERM_TAG, "Running tasks list: \n %s\r\n", tasklist);
#endif

#if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
        memset(tasklist, 0 , BUF_SIZE);
        // vTaskGetRunTimeStats(tasklist);
        ESP_LOGI(TERM_TAG, "Running tasks CPU usage: \n %s\r\n", tasklist);
#endif
        free(tasklist);
    }
}

#ifndef CONFIG_CLASSIC_BT_ENABLED
static void wifiInfo(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    wifi_config_t w_config;
    memset(&w_config, 0x00, sizeof(wifi_config_t));
    if (ESP_OK == esp_wifi_get_config(WIFI_IF_STA, &w_config)) {
        ESP_LOGI(TERM_TAG, "Connected Wifi SSID:\"%s\"", w_config.sta.ssid);
    } else {
        ESP_LOGE(TERM_TAG, "failed");
    }
}

static void wifiSet(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    wifi_config_t w_config;
    memset(&w_config, 0x00, sizeof(wifi_config_t));
    if (argc == 2) {
        memcpy(w_config.sta.ssid, argv[0], strlen(argv[0]));
        memcpy(w_config.sta.password, argv[1], strlen(argv[1]));
        esp_wifi_disconnect();
        ESP_LOGI(TERM_TAG, "Connected Wifi SSID:\"%s\" PASSWORD:\"%s\"", w_config.sta.ssid, w_config.sta.password);
        esp_wifi_set_config(WIFI_IF_STA, &w_config);
        esp_wifi_connect();
    } else {
        ESP_LOGE(TERM_TAG, "argc != 2");
    }
}
#endif


/*---------------------
|     Codec Commands   |
----------------------*/

static void muteon(void *ref, int argc, char *argv[])
{
    (void)argv;
    MediaHalSetMute(CODEC_MUTE_ENABLE);
    ESP_LOGI(TERM_TAG, "muteon=%d\r\n", MediaHalGetMute());
}

static void muteoff(void *ref, int argc, char *argv[])
{
    (void)argv;
    MediaHalSetMute(CODEC_MUTE_DISABLE);
    ESP_LOGI(TERM_TAG, "muteoff=%d\r\n", MediaHalGetMute());
}

static void auxon(void *ref, int argc, char *argv[])
{
    MediaHalStart(CODEC_MODE_LINE_IN);
    ESP_LOGI(TERM_TAG, "Aux In ON");
}

static void auxoff(void *ref, int argc, char *argv[])
{
    MediaHalStop(CODEC_MODE_LINE_IN);
    MediaHalStart(CODEC_MODE_DECODE);
    ESP_LOGI(TERM_TAG, "Aux In OFF");
}

static void esset(void *ref, int argc, char *argv[])
{
    if (argc == 2) {
        int curVol = 0, reg = 0;
        char num[10] = {0};
        strncpy(num, argv[1], 9);
        curVol = atoi(num);
        strncpy(num, argv[0], 9);
        reg = atoi(num);
        ESP_LOGI(TERM_TAG, "reg:%d set:%d\n", reg, curVol);
        EsWriteReg(reg, curVol);
    } else {
        ESP_LOGE(TERM_TAG, "in %s :argc != 2", __func__);
        return;
    }
}

static void esget(void *ref, int argc, char *argv[])
{
    Es8388ReadAll();
}


/*---------------------
|    Player Commands   |
----------------------*/

static void addUri(void *ref, int argc, char *argv[])
{
	PlayerStatus player = {0};
    TerminalControlService *term = (TerminalControlService *)ref;
    char *uri = NULL;
    if (argc == 1) {
        uri = argv[0];
    } else {
        ESP_LOGE(TERM_TAG, "Invalid param");
        return;
    }

	
	term->Based.stopTone(term);	
	xQueueReceive( xQuePlayerStatus, &player, portMAX_DELAY );
	  ESP_LOGW(TERM_TAG, "--------Mode:%d,Status:0x%02x,ErrMsg:%x --------",
			   player.mode, player.status, player.errMsg);
    if (term->Based.addUri) {
        term->Based.addUri((MediaService *)term, uri);
        term->Based.mediaPlay((MediaService *)term);
    }
    ESP_LOGI(TERM_TAG, "addUri: %s", uri);
}

static void play(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    (void)argv;
    ESP_LOGI(TERM_TAG, "play\r\n");
    // vTaskDelay(2000/portTICK_RATE_MS);
    if (term->Based.mediaPlay)
        term->Based.mediaPlay((MediaService *)term);
}

static void _pause(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    (void)argv;
    ESP_LOGI(TERM_TAG, "pause\r\n");
    if (term->Based.mediaPause)
        term->Based.mediaPause((MediaService *)term);
}

static void resume(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    (void)argv;
    ESP_LOGI(TERM_TAG, "resume\r\n");
    if (term->Based.mediaResume)
        term->Based.mediaResume((MediaService *)term);
}

static void stop(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    (void)argv;
    ESP_LOGI(TERM_TAG, "stop\r\n");
    if (term->Based.mediaStop)
        term->Based.mediaStop((MediaService *)term);
}

static void seekTo(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int time;
    if (argc == 1) {
        char num[10] = "";
        strncpy(num, argv[0], 9);
        time = atoi(num);
    } else {
        ESP_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    ESP_LOGI(TERM_TAG, "Ready seek to  %ds", time);
    int ret = term->Based.seekByTime((MediaService *)term, time);
    ESP_LOGI(TERM_TAG, "seekByTime ret=%d", ret) ;
}

static void getPlayingTime(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int ret = term->Based.getPosByTime((MediaService *)term);
    ESP_LOGI(TERM_TAG, "getPosByTime %d ms", ret);
    float duration = (float)ret / 1000.0;
    int hours = floor(duration / 3600);
    int minutes = floor((duration - (hours * 3600)) / 60);
    int seconds = duration - (hours * 3600) - (minutes * 60);
    ESP_LOGI(TERM_TAG, "Time is %02d:%02d:%02d",  hours, minutes, seconds);
}

static void setvol(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int curVol = 0;
    if (argc == 1) {
        char num[10] = "";
        strncpy(num, argv[0], 9);
        curVol = atoi(num);
    } else {
        ESP_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    int prevVol;
    prevVol = term->Based.getVolume((MediaService *)term);
    ESP_LOGI(TERM_TAG, "Prev value is %d", prevVol);
    term->Based.setVolume((MediaService *)term, 0, curVol);
    ESP_LOGI(TERM_TAG, "cur value is %d", curVol);
}

static void setUpsampling(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int sampleRate;
    if (argc == 1) {
        sampleRate = atoi(argv[0]);
    } else {
        ESP_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    ESP_LOGI(TERM_TAG, "setUpsampling %d", sampleRate);
    term->Based.upsamplingSetup(sampleRate);
}

static void setDecoder(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    char *str = NULL;
    if (argc == 1) {
        str = argv[0];
        ESP_LOGI(TERM_TAG, "str: %s", str);
    } else {
        ESP_LOGI(TERM_TAG, "set as NULL");
    }
    EspAudioSetDecoder(str);
}

// CMD: Playtone [URI/index] [TerminateType]
static void playtone(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    char *string;
    int index = 0;
    int termType = 0;
    if (argc > 0) {
        if (isdigit((int)argv[0][0])) {
            char num[10] = "";
            strncpy(num, argv[0], 9);
            index = atoi(num);
            if (index > getToneUriMaxIndex()) {
                ESP_LOGE(TERM_TAG, "index must be smaller than %d\n", getToneUriMaxIndex());
                return;
            }
            string = tone_uri[index];
            ESP_LOGI(TERM_TAG, "palytone index= %d", index);
        } else {
            ESP_LOGI(TERM_TAG, "palytone %s", argv[0]);
            string = argv[0];
        }
        // Terminate type
        if (argc == 2) {
            char num[10] = "";
            strncpy(num, argv[1], 9);
            termType = atoi(num);
            ESP_LOGI(TERM_TAG, "palytone termType= %d", termType);
        }

    } else {
        ESP_LOGE(TERM_TAG, "Invalid param, Support CMD: Playtone [URI/index]  or [terminateType]");
        return;
    }
    if (term->Based.playTone) {
        term->Based.playTone(term,  (const char *)string, termType);
    }
}


static void stoptone(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    if (term->Based.stopTone)
        ESP_LOGI(TERM_TAG, "Tone stop,ret=%x, \r\n", term->Based.stopTone((MediaService *)term));
}

static int rawTaskRun;
static void rawstart(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    //record the data from mic and save into TF-Card
    const char *recUri[] = {
        "i2s://8000:1@record.amr#file",
        "i2s://16000:1@record.opus#file",
        "i2s://16000:1@record.wav#file"
    };
    const char *playbackUri[] = {
        //Play the file recorded in TF-Card
        "file:///sdcard/__record/record.amr",
        "file:///sdcard/__record/record.wav",
    };
    char *string = NULL;
    if (argc > 0) {
        if (isdigit((int)argv[0][0])) {
            char num[10] = "";
            strncpy(num, argv[0], 9);
            int index = atoi(num);
            if (index > sizeof(recUri) / sizeof(char *)) {
                ESP_LOGE(TERM_TAG, "raw start index not corrected[%d]", index);
                return;
            }
            string = recUri[index];
            ESP_LOGI(TERM_TAG, "raw start  index= %d, str: %s", index, string);
        } else {
            ESP_LOGI(TERM_TAG, "raw start  %s", argv[0]);
            string = argv[0];
        }
    } else {
        //record the data from mic and save them into internal buffer, call `EspAudioRawRead()` to read the data from internal buffer
        string = "i2s://16000:1@record.pcm#raw";
        ESP_LOGI(TERM_TAG, "default raw URI: %s", string);
    }
    term->Based.rawStart(term, string);
}

static void rawstop(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int ret = 0;
    if (argc > 0) {
        char num[10] = "";
        strncpy(num, argv[0], 9);
        int index = atoi(num);
        ret = term->Based.rawStop(term, index);
        ESP_LOGI(TERM_TAG, "Raw data Stop[%s], ret= %d", index == 0 ? "TERMINATION_TYPE_NOW" : "TERMINATION_TYPE_DONE", ret);
    } else {
        ret = term->Based.rawStop(term, TERMINATION_TYPE_DONE);
        ESP_LOGI(TERM_TAG, "Raw Data Default Stop[TERMINATION_TYPE_DONE], ret= %d", ret);
    }
}

void rawReadTask(void *para)
{
    TerminalControlService *term = (TerminalControlService *) para;
    char *buf = EspAudioAlloc(1, 4096);
    //record the data from mic and save them into internal buffer,
    term->Based.rawStart(term, "i2s://16000:1@record.pcm#raw");
    ESP_LOGI(TERM_TAG, "Raw data reading...,%s", "i2s://16000:1@record.pcm#raw");
    int len = 0;
    while (rawTaskRun) {
        //to read the data from internal buffer
        int ret  = term->Based.rawRead(term, buf, 4096, &len);
        ESP_LOGI(TERM_TAG, "writing..");
        if (AUDIO_ERR_NOT_READY == ret) {
            ESP_LOGE(TERM_TAG, "AUDIO_ERR_NOT_READY");
            break;
        }
    }
    free(buf);
    ESP_LOGI(TERM_TAG, "Raw data read end");
    vTaskDelete(NULL);
}

static void rawread(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    ESP_LOGI(TERM_TAG, "Raw data reading...");
    rawTaskRun = 1;
    if (xTaskCreate(rawReadTask,
                    "rawReadTask",
                    TERM_SERV_TASK_STACK_SIZE,
                    term,
                    TERM_SERV_TASK_PRIORITY,
                    NULL) != pdPASS) {

        ESP_LOGE(TERM_TAG, "ERROR creating rawReadTask task! Out of memory?");
    }
}

void rawWriteTask(void *para)
{
    TerminalControlService *term = (TerminalControlService *) para;
    char *buf = EspAudioAlloc(1, 4096);
    //initialize the player as "buffer to i2s driver" mode
    term->Based.rawStart(term, "raw://from.pcm/to.pcm#i2s");
    ESP_LOGI(TERM_TAG, "Raw data writing...,%s", "raw://from.pcm/to.pcm#i2s");
    while (rawTaskRun) {
        //feed the music data into i2s driver
        if (term->Based.rawWrite(term, buf, 4096, NULL)) {
            break;
        }
        vTaskDelay(10);
    }
    free(buf);
    ESP_LOGI(TERM_TAG, "Raw data read end");
    vTaskDelete(NULL);
}
static void rawwrite(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    rawTaskRun = 1;

    if (xTaskCreate(rawWriteTask,
                    "rawWriteTask",
                    TERM_SERV_TASK_STACK_SIZE,
                    term,
                    TERM_SERV_TASK_PRIORITY,
                    NULL) != pdPASS) {

        ESP_LOGE(TERM_TAG, "ERROR creating rawWriteTask task! Out of memory?");
    }
}

static void getPlayingSongInfo(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    MusicInfo info = {0};
    PlaySrc src;
    int ret = term->Based.getSongInfo((MediaService *)term, &info);
    EspAudioPlayerSrcGet(&src);
    ESP_LOGI(TERM_TAG, "getSongInfo ret=%d", ret);
    ESP_LOGI(TERM_TAG, "source %d", src);
    if (info.name) {
        ESP_LOGI(TERM_TAG, "info.name=%s", info.name);
    }
    ESP_LOGI(TERM_TAG, "info.totalTime=%d ms", info.totalTime);
    ESP_LOGI(TERM_TAG, "info.totalBytes=%d byte", info.totalBytes) ;
    ESP_LOGI(TERM_TAG, "info.bitRates=%d bps", info.bitRates);
    ESP_LOGI(TERM_TAG, "info.sampleRates=%d Hz", info.sampleRates);
    ESP_LOGI(TERM_TAG, "info.channels=%d", info.channels);
    ESP_LOGI(TERM_TAG, "info.bits=%d bit", info.bits);
    if (info.dataLen > 0)
        ESP_LOGI(TERM_TAG, "data: %s, len: %d", info.data == NULL ? "NULL" : info.data, info.dataLen);
}



/*---------------------
|   Playlist Commands  |
----------------------*/

static void next(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;

    if (argc == 1) {
        ESP_LOGI(TERM_TAG, "next %d\r\n", atoi(argv[0]));

        for (int i = 0; i < atoi(argv[0]); i++) {
            term->Based.mediaNext((MediaService *)term);
        }
    } else  if (argc == 0) {
        ESP_LOGI(TERM_TAG, "next\r\n");
        term->Based.mediaNext((MediaService *)term);
    } else {
        ESP_LOGE(TERM_TAG, "Invalid param");
        return;
    }
}

static void prev(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    if (argc == 1) {
        ESP_LOGI(TERM_TAG, "prev %d\r\n", atoi(argv[0]));

        for (int i = 0; i < atoi(argv[0]); i++) {
            term->Based.mediaPrev((MediaService *)term);
        }
    } else  if (argc == 0) {
        ESP_LOGI(TERM_TAG, "prev\r\n");
        term->Based.mediaPrev((MediaService *)term);
    } else {
        ESP_LOGE(TERM_TAG, "Invalid param");
        return;
    }
}

static void setplaymode(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    if (argc == 1) {
        if (strncmp(argv[0], "random", strlen("random")) == 0) {
            ESP_LOGI(TERM_TAG, "mode set: random\n");
            term->Based.setPlayMode((MediaService *)term, MEDIA_PLAY_SHUFFLE);
        } else if (strncmp(argv[0], "seq", strlen("seq")) == 0) {
            ESP_LOGI(TERM_TAG, "mode set: sequential\n");
            term->Based.setPlayMode((MediaService *)term, MEDIA_PLAY_SEQUENTIAL);
        } else if (strncmp(argv[0], "repeat", strlen("repeat")) == 0) {
            ESP_LOGI(TERM_TAG, "mode set: one repeat\n");
            term->Based.setPlayMode((MediaService *)term, MEDIA_PLAY_REPEAT);
        } else if (strncmp(argv[0], "one", strlen("one")) == 0) {
            ESP_LOGI(TERM_TAG, "mode set: once\n");
            term->Based.setPlayMode((MediaService *)term, MEDIA_PLAY_ONE_SONG);
        } else {
            ESP_LOGI(TERM_TAG, "mode not support\n");
        }

    } else {
        ESP_LOGE(TERM_TAG, "in %s :argc != 1", __func__);
        return;
    }
}

static void getplaymode(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int mod = 0;
    int ret = term->Based.getPlayMode(term, &mod);
    ESP_LOGI(TERM_TAG, "Play mode %d,ret=%d\n", mod, ret);
}

static void setPlaylist(void *ref, int argc, char *argv[])
{
    char listId;
    if (argc == 1) {
        char num[10] = { 0 };
        strncpy(num, argv[0], 9);
        listId = atoi(num);
    } else {
        ESP_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    EspAudioPlaylistSet(listId, 0);
    ESP_LOGI(TERM_TAG, "Current ListID = %d", EspAudioCurListGet());
}

static void getPlaylistId(void *ref, int argc, char *argv[])
{
    ESP_LOGI(TERM_TAG, "Current ListID = %d", EspAudioCurListGet());
}

static void clearPlaylist(void *ref, int argc, char *argv[])
{
    char listId;
    if (argc == 1) {
        char num[10] = { 0 };
        strncpy(num, argv[0], 9);
        listId = atoi(num);
    } else {
        ESP_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    ESP_LOGI(TERM_TAG, "clear %d list", (int)listId);
    EspAudioPlaylistClear(listId);
}

static void closePlaylist(void *ref, int argc, char *argv[])
{
    char listId;
    if (argc == 1) {
        char num[10] = { 0 };
        strncpy(num, argv[0], 9);
        listId = atoi(num);
    } else {
        ESP_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    ESP_LOGI(TERM_TAG, "Close %d list", listId);
    EspAudioPlaylistClose(listId);
}

/*
    How to use:

        To save      @ saveuri 7 http://iot.espressif.com:8008/file/Windows.wav
*/
static void saveuri(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;

    if (argc == 2) {
        if (argv[strlen(argv[1])] == 'n' && argv[strlen(argv[1]) - 1] == '\\') {
            argv[strlen(argv[1]) - 1] = "\n";
            argv[strlen(argv[1])] = "\0";
        }
        ESP_LOGI(TERM_TAG, "save %s to %d", argv[1], atoi(argv[0]));
        EspAudioPlaylistSave(atoi(argv[0]), argv[1]);
    } else {
        ESP_LOGE(TERM_TAG, "argc != 1");
        return;
    }
}

/*
    How to use:

        To Read    @ readlist 500 10 7 0 (read 500 Bytes from the current playlist with 10 bytes offset in list 7)
                   @ readlist (any) 11 8 (any) (read the offset of 11th song from the current playlist attribution file in list 8)
*/
static void readPlaylist(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;

    if (argc == 4) {
        ESP_LOGI(TERM_TAG, "read %d bytes with %d offset in %d list", atoi(argv[0]), atoi(argv[1]), atoi(argv[2]));
        char *buf = calloc(1, atoi(argv[0]));
        if (buf == NULL) {
            ESP_LOGE(TERM_TAG, "malloc failed");
            return;
        }
        if (atoi(argv[3]) == 0) {
            EspAudioPlaylistRead(atoi(argv[2]), buf, atoi(argv[0]), atoi(argv[1]), 0);
            ESP_LOGI(TERM_TAG, "read %s end", buf);
        } else {
            int offset = 0;
            EspAudioPlaylistRead(atoi(argv[2]), &offset, sizeof(int), atoi(argv[1]), 1);
            ESP_LOGI(TERM_TAG, "offset is %d", offset);
        }
        free(buf);
    } else {
        ESP_LOGE(TERM_TAG, "argc != 4");
        return;
    }
}

void test(void *ref, int argc, char *argv[])
{
    PlayerStatus player = {0};
    TerminalControlService *term = (TerminalControlService *)ref;
    term->Based.playTone(term,  (const char *)"array:///upgrade_complete.mp3", 0);
    if (argc == 1) {
        vTaskDelay(atoi(argv[0]) / portTICK_RATE_MS);
        ESP_LOGE(TERM_TAG, "delay %dms", atoi(argv[0]));
    }
    xQueueReceive( xQuePlayerStatus, &player, portMAX_DELAY );
    ESP_LOGW(TERM_TAG, "--------Mode:%d,Status:0x%02x,ErrMsg:%x --------",
             player.mode, player.status, player.errMsg);

    term->Based.stopTone(term);
    xQueueReceive( xQuePlayerStatus, &player, portMAX_DELAY );
    ESP_LOGW(TERM_TAG, "--------Mode:%d,Status:0x%02x,ErrMsg:%x --------",
             player.mode, player.status, player.errMsg);
    term->Based.addUri((MediaService *)term, "http://cdnmusic.hezi.360iii.net/children/DataChildrenJoke/184496117653.mp3");
    term->Based.mediaPlay((MediaService *)term);
}


/*
    How to use:

         @ getlistinfo 7 (get list 7 info)
*/
static void getPlaylistInfo(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;

    if (argc == 1) {
        int size, amount;
        if (EspAudioPlaylistInfo(atoi(argv[0]), &amount, &size) == 0) {
            ESP_LOGI(TERM_TAG, "list id: %d,amount: %d pieces,size: %d bytes", atoi(argv[0]), amount, size);
        }
    } else {
        ESP_LOGE(TERM_TAG, "argc != 1");
        return;
    }
}

static void showtime(void)
{
	time_t time_sec   = time(NULL);
	char time_str[32] = {0};	

	if (time_sec == 0)
	{
		ESP_LOGE(TERM_TAG, "Time has not been calibrated!\n");
	}
	else
	{
		ESP_LOGE(TERM_TAG, "[%ld]\n", time_sec);
		time_sec = time_sec + 28800;
		struct tm info = *localtime(&time_sec);
		strftime(time_str, sizeof(time_str), "%Y年%m月%d日%H时%M分%S秒", &info);
		ESP_LOGE(TERM_TAG, "%s\n", time_str);
	}
}

static void showmemo(void)
{
	show_memo();
}

static void set_ota_mode(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    char *string;
    int index = 0;
    int termType = 0;
	OTA_UPDATE_MODE_T mode = OTA_UPDATE_MODE_FORMAL;
	
    if (argc > 0) 
	{
        if (strcmp(argv[0], "normal") == 0)
        {
        	ESP_LOGE(TERM_TAG, "otamode %s", argv[0]);
        	mode = OTA_UPDATE_MODE_FORMAL;
			set_flash_cfg(FLASH_CFG_OTA_MODE, &mode);
		}
		else if (strcmp(argv[0], "test") == 0)
		{
			ESP_LOGE(TERM_TAG, "otamode %s", argv[0]);
			mode = OTA_UPDATE_MODE_TEST;
			set_flash_cfg(FLASH_CFG_OTA_MODE, &mode);
		}
		else
		{
			ESP_LOGE(TERM_TAG, "Invalid param, Support CMD: otamode normal or otamode test");
		}
    } 
	else 
	{
        ESP_LOGE(TERM_TAG, "Invalid param, Support CMD: otamode normal or otamode test");
        return;
    }
}

static void set_asr_mode(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    char *string;
    int index = 0;
    int termType = 0;
	ASR_RECORD_TYPE_T mode = ASR_RECORD_TYPE_AMRNB;
	
    if (argc > 0) 
	{
        if (strcmp(argv[0], "0") == 0)
        {
        	ESP_LOGE(TERM_TAG, "asrmode %s", argv[0]);
        	mode = ASR_RECORD_TYPE_AMRNB;
			set_flash_cfg(FLASH_CFG_ASR_MODE, &mode);
		}
		else if (strcmp(argv[0], "1") == 0)
		{
			ESP_LOGE(TERM_TAG, "asrmode %s", argv[0]);
        	mode = ASR_RECORD_TYPE_AMRWB;
			set_flash_cfg(FLASH_CFG_ASR_MODE, &mode);
		}
		else if (strcmp(argv[0], "2") == 0)
		{
			ESP_LOGE(TERM_TAG, "asrmode %s", argv[0]);
        	mode = ASR_RECORD_TYPE_SPXNB;
			set_flash_cfg(FLASH_CFG_ASR_MODE, &mode);
		}
		else if (strcmp(argv[0], "3") == 0)
		{
			ESP_LOGE(TERM_TAG, "asrmode %s", argv[0]);
        	mode = ASR_RECORD_TYPE_SPXWB;
			set_flash_cfg(FLASH_CFG_ASR_MODE, &mode);
		}
		else if (strcmp(argv[0], "4") == 0)
		{
			ESP_LOGE(TERM_TAG, "asrmode %s", argv[0]);
        	mode = ASR_RECORD_TYPE_PCM_UNISOUND_16K;
			set_flash_cfg(FLASH_CFG_ASR_MODE, &mode);
		}
		else if (strcmp(argv[0], "5") == 0)
		{
			ESP_LOGE(TERM_TAG, "asrmode %s", argv[0]);
        	mode = ASR_RECORD_TYPE_PCM_SINOVOICE_16K;
			set_flash_cfg(FLASH_CFG_ASR_MODE, &mode);
		}
		else
		{
			ESP_LOGE(TERM_TAG, "Invalid param, Support CMD: asrmode 0,1,2,3,4,5");
		}
    } 
	else 
	{
        ESP_LOGE(TERM_TAG, "Invalid param, Support CMD: asrmode 0,1,2,3,4,5");
        return;
    }
}


const ShellCommand command[] = {
    //system
    {"------system-------", NULL},
    {"mem", getMem},
    {"reboot", reboot},
#ifndef CONFIG_CLASSIC_BT_ENABLED
    {"wifiInfo", wifiInfo},
    {"wifiSet", wifiSet},
#endif
    {"tasklist", tasklist}, // for debug
    {"sleep", terSleep},
    {"getback", backupinfoget}, // for debug
    {"SwitchBin", SwitchBin},
    {"showtime", showtime},
    {"showmemo", showmemo},

    //codec
    {"------codec-------", NULL},
    {"esset", esset},
    {"esget", esget},
    {"muteon", muteon},
    {"muteoff", muteoff},
    {"auxon", auxon},
    {"auxoff", auxoff},

    //playlist
    {"------playlist-------", NULL},
    {"setplaylist", setPlaylist},
    {"getplaylistid", getPlaylistId},
    {"clearplaylist", clearPlaylist},
    {"closelist", closePlaylist},
    {"saveuri", saveuri},
    {"readlist", readPlaylist},
    {"setplaymode", setplaymode},
    {"getplaymode", getplaymode},
    {"getlistinfo", getPlaylistInfo},
    {"next", next},
    {"prev", prev},

    //player
    {"------player-------", NULL},
    {"adduri", addUri},
    {"play", play},
    {"pause", _pause},
    {"resume", resume},
    {"stop", stop},
    {"seek", seekTo},
    {"gettime", getPlayingTime},
    {"setvol", setvol},
    {"setusp", setUpsampling},
    {"setDecoder", setDecoder},

    {"playtone", playtone},
    {"stoptone", stoptone},

    {"rawstart", rawstart},
    {"rawstop", rawstop},
    {"rawread", rawread},
    {"rawwrite", rawwrite},

    {"getsonginfo", getPlayingSongInfo},
	{"deviceinfo", print_device_params},
	{"otamode", set_ota_mode},
	{"asrmode", set_asr_mode},
    {NULL, NULL}
};

void terminalControlActive(TerminalControlService *service)
{
    shell_init(command, service);
	
	xQuePlayerStatus = xQueueCreate(2, sizeof(PlayerStatus));
  	configASSERT(xQuePlayerStatus);
  	service->Based.addListener((MediaService *)service, xQuePlayerStatus);
}
void terminalControlDeactive(TerminalControlService *service)
{
    ESP_LOGI(TERM_TAG, "terminalControlStop\r\n");
    shell_stop();
}

TerminalControlService *TerminalControlCreate()
{
    TerminalControlService *term = (TerminalControlService *) calloc(1, sizeof(TerminalControlService));
    ESP_ERROR_CHECK(!term);

    term->Based.playerStatusUpdated = playerStatusUpdatedToTerm;
    term->Based.serviceActive = terminalControlActive;
    term->Based.serviceDeactive = terminalControlDeactive;

    return term;
}
