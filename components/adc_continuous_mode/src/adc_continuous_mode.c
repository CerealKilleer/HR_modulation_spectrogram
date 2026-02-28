#include "adc_continuous_mode.h"
#include "esp_adc/adc_continuous.h"
#include "esp_err.h"
#include "hal/adc_types.h"

void adc_init(adc_continuous_params_t *params)
{
    adc_channel_t adc_channel;
    adc_unit_t adc_unit;

    adc_continuous_handle_cfg_t hdl_config = {
        .max_store_buf_size = params->max_buf_size,
        .conv_frame_size = params->conv_frame_size
    };

    ESP_ERROR_CHECK(adc_continuous_new_handle(&hdl_config, &params->adc_handler));
    ESP_ERROR_CHECK(adc_continuous_io_to_channel(params->adc_gpio, &adc_unit, &adc_channel));
    
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
    ESP_ERROR_CHECK(adc_continuous_config(params->adc_handler, &dig_cfg));
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(params->adc_handler, &params->cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(params->adc_handler));
}