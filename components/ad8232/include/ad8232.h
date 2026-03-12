#ifndef __APP_H__
#define __APP_H__

#include <stdint.h>
#include "adc_continuous_mode.h"
#include "esp_adc/adc_continuous.h"


typedef struct {
    adc_config_t adc_continuous_cfg;
} ad8232_adc_config_t;

typedef struct {
    uint8_t *conv_frame;
    adc_continuous_data_t *parsed_data;
    uint16_t *buffer;
    const uint32_t buffer_size;
    uint32_t conv_frame_size;
    adc_continuous_handle_t adc_handle;
    const uint8_t oversampling;
} ad8232_adc_samples_t;

bool ad8232_init(ad8232_adc_config_t *params);
int ad8232_get_samples(ad8232_adc_samples_t *adc_samples);
inline bool ad8232_adc_start(adc_continuous_handle_t adc_handle) 
{
    return adc_start(adc_handle);
}
#endif