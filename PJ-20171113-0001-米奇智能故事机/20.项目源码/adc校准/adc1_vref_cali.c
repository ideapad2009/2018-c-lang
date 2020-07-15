#include "esp_adc_cal.h"
#include "esp_efuse.h"
#include "esp_log.h"

#define EFUSE_TEST 1  // For Debug,1: it will not write EFUSE


#define DEFAULT_V_REF   1100
#define ADC_SAMPLES_NUM 30


static const char* TAG = "efuse";
static uint32_t Vref_eFuse;
static esp_adc_cal_characteristics_t s_characteristics;
static adc1_channel_t s_adc1_ch;

int cst_b[4] = {59, 63, 83, 116};
int cst_a[4] = {874, 1162, 1612, 3013};


static void esp_efuse_write_vref(uint32_t vol)
{
#if EFUSE_TEST
    Vref_eFuse = vol;
#else
    esp_efuse_reset();
    if ((REG_READ(EFUSE_BLK0_RDATA4_REG) & EFUSE_RD_VREF_M) == 0) {
        ESP_LOGI(TAG, "Disable BASIC ROM Console fallback via efuse...");
        esp_efuse_reset();
        REG_WRITE(EFUSE_BLK0_WDATA4_REG, vol << EFUSE_WR_VREF_S );
        esp_efuse_burn_new_values();
    }
#endif
}

static uint32_t esp_efuse_read_vref(void)
{
#if EFUSE_TEST
    return Vref_eFuse;
#else
    esp_efuse_reset();
    return (REG_READ(EFUSE_BLK0_RDATA4_REG) & EFUSE_RD_VREF_M) >> EFUSE_RD_VREF_S;
#endif
}

uint32_t esp_efuse_vref_is_cali(void)
{
    uint32_t k = 0;
#if EFUSE_TEST
    k =  Vref_eFuse;
#else
    esp_efuse_reset();
    k = (REG_READ(EFUSE_BLK0_RDATA4_REG) & EFUSE_RD_VREF_M) >> EFUSE_RD_VREF_S;
#endif
    if (0 == k) {
        return 0; // Not set
    } else {
        return 1;
    }
}

static uint32_t vref_to_efuse_cali(uint32_t vref)
{
    int k = vref - DEFAULT_V_REF;
    uint32_t res = k == 0 ? 0x10 : (k & 0x80000000) >> 27;
    if (k < 0) {
        k = -k;
    }
    k = k * 10 / 7 + 5;
    res += ((k / 10) & 0x0F);
    printf("vref_to_efuse_cali,k:%d, res:%x\r\n", k, res);
    return res;
}

static uint32_t efuse_cali_to_vref(uint32_t cali)
{
    uint32_t k = cali & 0x01F;
    uint32_t res = 0;
    if (k & 0x10) {
        res = (k & 0x0F) * 7;
        res = DEFAULT_V_REF - res;
    } else {
        res = (k & 0x0F) * 7;
        res = DEFAULT_V_REF + res;
    }
    printf("efuse_cali_to_vref,k:%d, res:%d\r\n", k, res);
    return res;
}

int adc1_cali_vref_set(uint32_t cst_voltage, uint32_t meas_value, adc_atten_t atten)
{
    uint32_t k  = 4096 * 1000 / cst_a[atten];
    if (!meas_value) {
        ESP_LOGE("ADC", "meas_value is zero,%d,%d,%d", cst_voltage, meas_value, atten);
        return -1;
    }
    uint32_t vol = (cst_voltage - cst_b[atten]) * k / meas_value;
    printf("Cali Vref:%d\r\n", vol);
    if (vol <= 1220 && vol >= 1200) {
        vol = 1200;
    }
    if (vol <= 1000 && vol >= 980) {
        vol = 1000;
    }
    if (vol < 1000
        || vol > 1200) {
        ESP_LOGE("ADC", "Get cali vref failed,vol:%d,%d,%d,%d", vol, cst_voltage, meas_value, atten);
        return -1;
    }
    // Get efuse value by vref
    k = vref_to_efuse_cali(vol);

    // Wirte value to efuse
    esp_efuse_write_vref(k);
    printf("eFuse value:%x\r\n", k);
    return 0;
}

uint32_t adc1_cali_vref_get(void)
{
    uint32_t res = esp_efuse_read_vref();
    uint32_t vref = efuse_cali_to_vref(res);
    printf("Refernce voltage:%d\r\n", vref);
    return vref;
}

uint32_t adc1_raw_filter(adc1_channel_t ch)
{
    uint32_t data[ADC_SAMPLES_NUM] = { 0 };
    uint32_t adc = 0;
    uint32_t sum = 0;
    int tmp = 0;
    for (int i = 0; i < ADC_SAMPLES_NUM; ++i) {
        data[i] = adc1_get_raw(ch);
    }
    for (int j = 0; j < ADC_SAMPLES_NUM - 1; j++) {
        for (int i = 0; i < ADC_SAMPLES_NUM - j - 1; i++) {
            if (data[i] > data[i + 1]) {
                tmp = data[i];
                data[i] = data[i + 1];
                data[i + 1] = tmp;
            }
        }
    }
    for (int num = 1; num < ADC_SAMPLES_NUM - 1; num++)
        sum += data[num];
    return (sum / (ADC_SAMPLES_NUM - 2));

}

int get_adc_voltage()
{
    uint32_t data[ADC_SAMPLES_NUM] = { 0 };
    uint32_t adc = 0;
    uint32_t sum = 0;
    int tmp = 0;
    for (int i = 0; i < ADC_SAMPLES_NUM; ++i) {
        data[i] = adc1_to_voltage(s_adc1_ch, &s_characteristics);
    }
    for (int j = 0; j < ADC_SAMPLES_NUM - 1; j++) {
        for (int i = 0; i < ADC_SAMPLES_NUM - j - 1; i++) {
            if (data[i] > data[i + 1]) {
                tmp = data[i];
                data[i] = data[i + 1];
                data[i + 1] = tmp;
            }
        }
    }
    for (int num = 1; num < ADC_SAMPLES_NUM - 1; num++)
        sum += data[num];
    return (sum / (ADC_SAMPLES_NUM - 2));
}

void test_vref(int vol)
{
    int num = vref_to_efuse_cali(vol);
    efuse_cali_to_vref(num);
}

void esp_adc1_vref_calibrate(adc_bits_width_t bits, adc_atten_t atten, adc1_channel_t ch, uint32_t cst_voltage)
{
    adc1_config_width(bits);
    adc1_config_channel_atten(ch, atten);
    adc1_cali_vref_set(cst_voltage, adc1_raw_filter(ch), atten);
}

void esp_adc1_cali_config(adc_bits_width_t bits, adc_atten_t atten, adc1_channel_t ch)
{
    adc1_config_width(bits);
    adc1_config_channel_atten(ch, atten);
    uint32_t vref = adc1_cali_vref_get();
    s_adc1_ch = ch;
    esp_adc_cal_get_characteristics(vref, atten, bits, &s_characteristics);
}