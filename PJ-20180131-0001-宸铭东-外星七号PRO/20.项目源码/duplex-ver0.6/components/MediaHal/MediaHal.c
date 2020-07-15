/*
*
* Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "ES8388_interface.h"
#include "MediaHal.h"
#include "driver/i2s.h"
#include "lock.h"
#include "userconfig.h"
#include "InterruptionSal.h"
#include "free_talk.h"

#define HAL_TAG "MEDIA_HAL"

#define MEDIA_HAL_CHECK_NULL(a, format, b, ...) \
    if ((a) == 0) { \
        ESP_LOGE(HAL_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

static CodecMode _currentMode;
static xSemaphoreHandle _halLock;
static MediaHalState sMediaHalState;

const i2s_pin_config_t i2s_pin = {
    .bck_io_num = IIS_SCLK,
    .ws_io_num = IIS_LCLK,
    .data_out_num = IIS_DSIN,
    .data_in_num = IIS_DOUT
};

#define I2S_OUT_VOL_DEFAULT     70
#define I2S_OUT_START_VOL      	70
#define SUPPOERTED_BITS 16
#define I2S1_ENABLE     1   // Enable i2s1
#define I2S_DAC_EN      0  //if enabled then a speaker can be connected to i2s output gpio(GPIO25 and GND or GPIO26 and GND), using DAC(8bits) to play music
#define I2S_ADC_EN      0  //NOT SUPPORTED before idf 3.0. if enabled then a mic can be connected to i2s input gpio, using ADC to record 8bit-music

static int I2S_NUM = I2S_NUM_0;//only support 16 now and i2s0 or i2s1
static char MUSIC_BITS = 16; //for re-bits feature, but only for 16 to 32

i2s_config_t i2s_config[] = {
    {
#if I2S_DAC_EN == 1
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
//    #else if I2S_ADC_EN == 1 //NOT SUPPORTED before idf 3.0.
//    .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_ADC_BUILT_IN,
#else
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX,
#endif
        .sample_rate = 48000,
        .bits_per_sample = SUPPOERTED_BITS,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
#if I2S_DAC_EN == 1
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
#else
        .communication_format = I2S_COMM_FORMAT_I2S,
#endif
        //when dma_buf_count = 3 and dma_buf_len = 300, then 3 * 4 * 300 * 2 Bytes internal RAM will be used. The multiplier 2 is for Rx buffer and Tx buffer together.
        .dma_buf_count = 3,                            /*!< amount of the dam buffer sectors*/
        .dma_buf_len = 300,                            /*!< dam buffer size of each sector (word, i.e. 4 Bytes) */
        .intr_alloc_flags = I2S_INTER_FLAG,
#if I2S_DAC_EN == 0
        .use_apll = 1,
#endif
    },
    {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX,
        .sample_rate = 48000,
        .bits_per_sample = SUPPOERTED_BITS,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        //when dma_buf_count = 3 and dma_buf_len = 300, then 3 * 4 * 300 * 2 Bytes internal RAM will be used. The multiplier 2 is for Rx buffer and Tx buffer together.
        .dma_buf_count = 3,                            /*!< amount of the dam buffer sectors*/
        .dma_buf_len = 300,                            /*!< dam buffer size of each sector (word, i.e. 4 Bytes) */
        .intr_alloc_flags = I2S_INTER_FLAG,
        .use_apll = 1,
    },
};

// Set ES8388
Es8388Config initConf = {
    .esMode = ES_MODE_SLAVE,
    .i2c_port_num = I2C_NUM_0,
    .i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = IIC_DATA,
        .scl_io_num = IIC_CLK,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    },
#if DIFFERENTIAL_MIC
    .adcInput = ADC_INPUT_DIFFERENCE,
#else
    .adcInput = ADC_INPUT_LINPUT1_RINPUT1,
#endif
    .dacOutput = DAC_OUTPUT_LOUT1 | DAC_OUTPUT_LOUT2 | DAC_OUTPUT_ROUT1 | DAC_OUTPUT_ROUT2,
};


int MediaHalInit(void)
{
    int ret  = 0;
    // Set I2S pins
    ret = i2s_driver_install(I2S_NUM, &i2s_config[I2S_NUM], 0, NULL);
    if (I2S_NUM > 1 || I2S_NUM < 0) {
        ESP_LOGE(HAL_TAG, "Must set I2S_NUM as 0 or 1");
        return -1;
    }

#if I2S1_ENABLE
    ret = i2s_driver_install(I2S_NUM_1, &i2s_config[I2S_NUM_1], 0, NULL);
    if (ret < 0) {
        ESP_LOGE(HAL_TAG, "I2S_NUM_1 install failed");
        return -1;
    }
#endif

#if I2S_DAC_EN == 1
    //init DAC pad
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
#else
    ret |= i2s_set_pin(I2S_NUM, &i2s_pin);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO35_U, FUNC_GPIO35_GPIO35);
    SET_PERI_REG_MASK(PERIPHS_IO_MUX_GPIO35_U, FUN_IE | FUN_PU);
    PIN_INPUT_ENABLE(PERIPHS_IO_MUX_GPIO35_U);

    ret |= Es8388Init(&initConf);
    ret |= Es8388ConfigFmt(ES_MODULE_ADC_DAC, ES_I2S_NORMAL);
    ret |= Es8388SetBitsPerSample(ES_MODULE_ADC_DAC, BIT_LENGTH_16BITS);
#if MODULE_DAC_DEFFERENT_OUT
	Es8388DefferenceConfig();
#endif
    if (I2S_NUM == 0) 
	{
        SET_PERI_REG_BITS(PIN_CTRL, CLK_OUT1, 0, CLK_OUT1_S);
    }
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
#endif

    _currentMode = CODEC_MODE_UNKNOWN;
    if (_halLock) {
        mutex_destroy(_halLock);
    }
    _halLock = mutex_init();

#if WROAR_BOARD
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_SEL_PA_EN;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_PA_EN, 0);
#endif

    MediaHalSetVolume(I2S_OUT_VOL_DEFAULT);
    ESP_LOGI(HAL_TAG, "I2S_OUT_VOL_DEFAULT[%d]", I2S_OUT_VOL_DEFAULT);

    sMediaHalState = MEDIA_HAL_STATE_INIT;
    return ret;
}

int MediaHalUninit(void)
{
    mutex_destroy(_halLock);
#if I2S_DAC_EN == 0
    Es8388Uninit(&initConf);
#endif
    _halLock = NULL;
    MUSIC_BITS = 0;
    _currentMode = CODEC_MODE_UNKNOWN;
    sMediaHalState = MEDIA_HAL_STATE_UNKNOWN;
    return 0;
}

int MediaHalStart(CodecMode mode)
{
    int ret = 0;
	
	MediaHalPaPwr(1);
	free_talk_auto_stop();
#if I2S_DAC_EN == 0
    int esMode = 0;
    mutex_lock(_halLock);
    switch (mode) {
    case CODEC_MODE_ENCODE:
        esMode  = ES_MODULE_ADC;
        break;
    case CODEC_MODE_LINE_IN:
        esMode  = ES_MODULE_LINE;
        break;
    case CODEC_MODE_DECODE:
        esMode  = ES_MODULE_DAC;
        break;
    case CODEC_MODE_DECODE_ENCODE:
        esMode  = ES_MODULE_ADC_DAC;
        break;
    default:
        esMode = ES_MODULE_DAC;
        ESP_LOGW(HAL_TAG, "Codec mode not support, default is decode mode");
        break;
    }
    ESP_LOGI(HAL_TAG, "Codec mode is %d", esMode);
    int inputConfig;
    if (esMode == ES_MODULE_LINE) {
        inputConfig = ADC_INPUT_LINPUT2_RINPUT2;
    } else {
#if DIFFERENTIAL_MIC
        inputConfig = ADC_INPUT_DIFFERENCE;
#else
        inputConfig = ADC_INPUT_LINPUT1_RINPUT1;
#endif
    }
    ESP_LOGI(HAL_TAG, "Codec inputConfig is %2X", inputConfig);
    ret = Es8388ConfigAdcInput(inputConfig);
    ret |= Es8388Start(esMode);
    _currentMode = mode;
	ets_delay_us(300*1000);
    mutex_unlock(_halLock);
#endif /* I2S_DAC_EN */
    return ret;
}

int MediaHalStop(CodecMode mode)
{
    int ret = 0;
	static int first_time_close = 1;

	if (first_time_close)
	{
		MediaHalSetVolume(I2S_OUT_START_VOL);
		first_time_close = 0;
	}
	
	MediaHalPaPwr(0);
	free_talk_auto_start();
#if I2S_DAC_EN == 0
    int esMode = 0;
    mutex_lock(_halLock);
    switch (mode) {
    case CODEC_MODE_ENCODE:
        esMode  = ES_MODULE_ADC;
        break;
    case CODEC_MODE_LINE_IN:
        esMode  = ES_MODULE_LINE;
        break;
    case CODEC_MODE_DECODE:
        esMode  = ES_MODULE_DAC;
        break;
    default:
        esMode = ES_MODULE_DAC;
        ESP_LOGI(HAL_TAG, "Codec mode not support");
        break;
    }
    ret = Es8388Stop(esMode);
    _currentMode = CODEC_MODE_UNKNOWN;
    mutex_unlock(_halLock);
#endif
    return ret;
}

int MediaHalGetCurrentMode(CodecMode *mode)
{
    MEDIA_HAL_CHECK_NULL(mode, "Get current mode para is null", -1);
    *mode = _currentMode;
    return 0;
}

int MediaHalSetVolume(int volume)
{
    int ret = 0;
#if I2S_DAC_EN == 0
    if (volume < 3 ) {
        if (0 == MediaHalGetMute()) {
            MediaHalSetMute(CODEC_MUTE_ENABLE);
        }
    } else {
        if ((1 == MediaHalGetMute())) {
            MediaHalSetMute(CODEC_MUTE_DISABLE);
        }
    }
    mutex_lock(_halLock);
    ret = Es8388SetVoiceVolume(volume);
    mutex_unlock(_halLock);
#endif
    return ret;
}

int MediaHalGetVolume(int *volume)
{
    int ret = 0;
    MEDIA_HAL_CHECK_NULL(volume, "Get volume para is null", -1);
#if I2S_DAC_EN == 0
    mutex_lock(_halLock);
    ret = Es8388GetVoiceVolume(volume);
    mutex_unlock(_halLock);
#endif
    return ret;
}

int MediaHalSetMute(CodecMute mute)
{
    int ret = 0;
#if I2S_DAC_EN == 0
    mutex_lock(_halLock);
    ret = Es8388SetVoiceMute(mute);
    mutex_unlock(_halLock);
#endif
    return ret;
}

int MediaHalGetMute(void)
{
    int res = 0;
#if I2S_DAC_EN == 0
    mutex_lock(_halLock);
    res = Es8388GetVoiceMute();
    mutex_unlock(_halLock);
#endif
    return res;
}

int MediaHalSetBits(int bitPerSample)
{
    int ret = 0;
    if (bitPerSample <= BIT_LENGTH_MIN || bitPerSample >= BIT_LENGTH_MAX) {
        ESP_LOGE(HAL_TAG, "bitPerSample: wrong param");
        return -1;
    }
#if I2S_DAC_EN == 0
    mutex_lock(_halLock);
    ret = Es8388SetBitsPerSample(ES_MODULE_ADC_DAC, (BitsLength)bitPerSample);
    mutex_unlock(_halLock);
#endif
    return ret;
}

int MediaHalSetClk(int i2s_num, uint32_t rate, uint8_t bits, uint32_t ch)
{
    int ret;
    if (bits != 16 && bits != 32) {
        ESP_LOGE(HAL_TAG, "bit should be 16 or 32, Bit:%d", bits);
        return -1;
    }
    if (ch != 1 && ch != 2) {
        ESP_LOGE(HAL_TAG, "channel should be 1 or 2 %d", ch);
        return -1;
    }
    if (bits > SUPPOERTED_BITS) {
        ESP_LOGE(HAL_TAG, "Bits:%d, bits must be smaller than %d", bits, SUPPOERTED_BITS);
        return -1;
    }

    MUSIC_BITS = bits;
    ret = i2s_set_clk((i2s_port_t)i2s_num, rate, SUPPOERTED_BITS, ch);

    return ret;
}

int MediaHalGetI2sConfig(int i2sNum, void *info)
{
    if (info) {
        memcpy(info, &i2s_config[i2sNum], sizeof(i2s_config_t));
    }
    return 0;
}

int MediaHalGetI2sNum(void)
{
    return I2S_NUM;
}

int MediaHalGetI2sBits(void)
{
    return SUPPOERTED_BITS;
}

int MediaHalGetSrcBits(void)
{
    return MUSIC_BITS;
}

int MediaHalGetI2sDacMode(void)
{
    return I2S_DAC_EN;
}

int MediaHalGetI2sAdcMode(void)
{
    return I2S_ADC_EN;
}

int MediaHalPaPwr(int en)
{
	static int mode = 0;
#if WROAR_BOARD
	if (en == -1)
	{
		mode = 1;
		gpio_set_level(GPIO_PA_EN, 0);
	}

	if (mode == 1)
	{
		return;
	}
	
    if (en) {
        gpio_set_level(GPIO_PA_EN, 1);
    } else {
        gpio_set_level(GPIO_PA_EN, 0);
    }
#endif
    return 0;
}

int MediaHalGetState(MediaHalState *state)
{
    if (state) {
        *state = sMediaHalState;
        return 0;
    }
    return -1;
}
