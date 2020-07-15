#ifndef __ADC1_VREF_CALI_H_
#define __ADC1_VREF_CALI_H_

#include "driver/adc.h"

uint32_t esp_efuse_vref_is_cali(void);
void test_vref(int vol);
void esp_adc1_vref_calibrate(adc_bits_width_t bits, adc_atten_t atten, adc1_channel_t ch, uint32_t cst_voltage);
void esp_adc1_cali_config(adc_bits_width_t bits, adc_atten_t atten, adc1_channel_t ch);
int get_adc_voltage(void);
#endif