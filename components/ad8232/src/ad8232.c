#include "ad8232.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "ad8232";

bool ad8232_init(ad8232_adc_config_t *params)
{
    bool ret;
    ret = adc_init(&params->adc_continuous_cfg);

    return ret;
}

int ad8232_get_samples(ad8232_adc_samples_t *adc_samples)
{
    static uint16_t buffer_idx = 0;
    static uint8_t mqtt_buffer_idx = 0;
    const uint32_t buffer_size = adc_samples->buffer_size;
    const uint32_t conv_frame_size = adc_samples->conv_frame_size;
    const uint8_t oversampling_value = adc_samples->oversampling;
    uint8_t cnt = 0;
    uint16_t sample = 0;
    uint32_t read_data_lenght;
    uint32_t parsed_data_lenght;
    uint8_t *conv_frame = adc_samples->conv_frame;
    adc_continuous_data_t *parsed_data = adc_samples->parsed_data;
    uint16_t *buffer = adc_samples->buffer;
    uint16_t *mqtt_buffer = adc_samples->mqtt_buffer;

    if (!adc_read(adc_samples->adc_handle, conv_frame, conv_frame_size, &read_data_lenght)) {
        ESP_LOGI(TAG, "La conversion del frame ha fallado");
        return -1;
    }

    esp_err_t parse_ret = adc_continuous_parse_data(adc_samples->adc_handle, conv_frame, read_data_lenght, parsed_data, &parsed_data_lenght);
    if (parse_ret != ESP_OK) {
        ESP_LOGE(TAG, "Error parseando los datos del ADC");
        return -1;
    }
        
    for (uint16_t i = 0; i < parsed_data_lenght; i++) {
        sample += parsed_data[i].raw_data;
        cnt++;
            
        if (cnt != oversampling_value) {
            continue;
        }

        sample = sample / oversampling_value;
        buffer[buffer_idx++] = sample;
        mqtt_buffer[mqtt_buffer_idx++] = sample;
        cnt = 0;
        sample = 0;

        if (mqtt_buffer_idx == MQTT_BUFFER_SIZE) {
            mqtt_buffer_idx = 0;
            adc_samples->mqtt_buffer_ready = true;
        }

        if (buffer_idx == buffer_size) {
            buffer_idx = 0;
        }
    }
    
    return buffer_idx;
}