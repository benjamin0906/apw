#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_2
#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12
#define ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED 1

static const char *TAG = "adc_example";
adc_cali_handle_t adc1_cali_chan0_handle = NULL;
adc_oneshot_unit_handle_t handle;

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config1 = {.unit_id = ADC_UNIT_1,};
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    adc_oneshot_new_unit(&init_config1, &handle);

    adc_oneshot_config_channel(handle, EXAMPLE_ADC1_CHAN0, &config);

    example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN0, EXAMPLE_ADC_ATTEN, &adc1_cali_chan0_handle);
}

uint16_t adc_get_raw(void)
{
    int ret = 0;
    adc_oneshot_read(handle, EXAMPLE_ADC1_CHAN0, &ret);
    return (uint16_t)ret;
}

uint16_t adc_get_volt(void)
{
    int voltage = 0;
    uint16_t raw = adc_get_raw();
    adc_cali_raw_to_voltage(adc1_cali_chan0_handle, raw, &voltage);
    return (uint16_t)voltage;
}

