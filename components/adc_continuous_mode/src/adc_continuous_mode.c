#include "adc_continuous_mode.h"
#include "esp_adc/adc_continuous.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/adc_types.h"

static const char *TAG = "adc_continuous";

bool adc_init(adc_config_t *params)
{
    esp_err_t ret;

    adc_channel_t adc_channel;
    adc_unit_t adc_unit;

    adc_continuous_handle_cfg_t hdl_config = {
        .max_store_buf_size = params->max_buf_size,
        .conv_frame_size = params->conv_frame_size
    };

    ret = adc_continuous_new_handle(&hdl_config, &params->adc_handler);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando el ADC %d", ret);
        goto exit_failure;
    }

    ret = adc_continuous_io_to_channel(params->adc_gpio, &adc_unit, &adc_channel);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando el ADC %d", ret);
        goto deinit;
    }
    
    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = params->sample_freq,
        .conv_mode = params->conv_mode,
        .pattern_num = 1,
    };

    adc_digi_pattern_config_t adc_pattern[1] = {0};
    
    adc_pattern[0].atten = params->atten;
    adc_pattern[0].channel = adc_channel;
    adc_pattern[0].unit = adc_unit;
    adc_pattern[0].bit_width = params->bit_width;

    dig_cfg.adc_pattern = adc_pattern;

    ret = adc_continuous_config(params->adc_handler, &dig_cfg);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error aplicando la configuración en el ADC %d", ret);
        goto deinit;
    }

    ret = adc_continuous_register_event_callbacks(params->adc_handler, &params->cbs, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error registrando el callback en el ADC %d", ret);
        goto deinit;
    }

    return true;

    deinit:
        adc_continuous_deinit(params->adc_handler);

    exit_failure:
        return false;
}