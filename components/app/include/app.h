#ifndef __APP_H__
#define __APP_H__

#include <stdint.h>
#include "esp_adc/adc_continuous.h"


typedef struct {
    adc_continuous_handle_t adc_handler;
    uint8_t gpio;
} ad8232_t;

void ad8232_init(ad8232_t *params);

#endif