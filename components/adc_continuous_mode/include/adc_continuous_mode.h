#ifndef __ADC_CONTINUOUS_MODE_H__
#define __ADC_CONTINUOUS_MODE_H__

#include "esp_adc/adc_continuous.h"
#include <stdint.h>

typedef struct {
    uint32_t max_buf_size;
    uint32_t conv_frame_size;
    uint32_t sample_freq;
    adc_continuous_evt_cbs_t cbs;
    adc_continuous_handle_t adc_handler;
    adc_digi_convert_mode_t conv_mode;
    uint8_t adc_gpio;
    uint8_t atten;
    uint8_t bit_width;
} adc_continuous_params_t;

void adc_init(adc_continuous_params_t *params);

bool inline adc_read(adc_continuous_handle_t adc_handle, uint8_t *buffer, 
                    uint32_t lenght_max, uint32_t *out_lenght)
{
    return (adc_continuous_read(adc_handle, buffer, lenght_max, out_lenght, 0) == ESP_OK);
}
#endif