#include "Es8388_I2s_Test.h"
#include <string.h>
#include "ES8388_interface.h"
#include "freertos/FreeRTOS.h"
#include "music_data.h"
#include "esp_system.h"
#include "ES8388_interface.h"
#include "driver/i2s.h"
#include "music_data.h"
#include "userconfig.h"

#define I2S_TEST_NUM 100*1024

const i2s_pin_config_t i2s_pin = {
    .bck_io_num = IIS_SCLK,
    .ws_io_num = 25,
    .data_out_num = IIS_DSIN,
    .data_in_num = 35
};
void EsI2sTest(void)
{
//  psram_enable(PSRAM_CACHE_F40M_S40M);
//    psram_add_block((void*)PSRAM_START_ADDR, PSRAM_SIZE);
    int i = 0;
    char *psram_buffer = (char*)malloc(I2S_TEST_NUM);
//  char *psram_buffer=(char*)PSRAM_START_ADDR;

    uint16_t *psram_buffer_16 = (uint16_t *)psram_buffer;
    memset(psram_buffer, 0, I2S_TEST_NUM);
//    vTaskDelay(5000/portTICK_RATE_MS);
    // I2sStreamCreate(&i2sStreamCfg2);
    Es8388Start(ES_MODULE_ADC_DAC);
    Es8388SetVoiceVolume(50);
    while (1) {
        i2s_write_bytes(0, StartupPcmData, sizeof(StartupPcmData), portMAX_DELAY);
    }
    while (1) {

        printf("i:%d\n", i);
        if (i == 0)
            Es8388Stop(ES_MODULE_ADC);
        if (i == 1)
            Es8388Start(ES_MODULE_ADC);

        if (i == 2)
            Es8388Stop(ES_MODULE_DAC);
        if (i == 3)
            Es8388Start(ES_MODULE_DAC);

        if (i == 4)
            Es8388Stop(ES_MODULE_ADC_DAC);
        if (i == 5)
            Es8388Start(ES_MODULE_ADC_DAC);

        if (i == 6)
            Es8388Stop(ES_MODULE_LINE);
        if (i == 7)
            Es8388Start(ES_MODULE_LINE);

        i++;
        if (i >= 8)
            i = 0;

        i2s_read_bytes(0, psram_buffer, I2S_TEST_NUM, portMAX_DELAY);

//        for(int i=0;i<(I2S_TEST_NUM);i++)
//        {
//            //printf("%02x ", psram_buffer_16[i]);
//            printf("%02x ", psram_buffer[i]);
//            if(i%100==0)
//                vTaskDelay(1);
//        }
//        vTaskDelay(5000/portTICK_RATE_MS);
//        printf("132\n");


        i2s_write_bytes(0, psram_buffer, I2S_TEST_NUM, portMAX_DELAY);

//        while(1)
//             vTaskDelay(1);
    }
}