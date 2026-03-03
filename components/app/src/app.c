#include "app.h"
#include "wifi.h"
#include "tcp_client.h"
#include "esp_adc/adc_continuous.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "adc_continuous_mode.h"
#include "hal/adc_types.h"
#include "modulation_spectrogram.h"
#include "portmacro.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include <stdio.h>

TaskHandle_t ad8232_conv_done_handle;
TaskHandle_t ad8232_process_samples_handle;
TaskHandle_t tcp_client_handle;

void ad8232_conv_done(void *params);
void ad8232_process_samples(void *params); 

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    vTaskNotifyGiveFromISR(ad8232_conv_done_handle, &mustYield);
    return (mustYield == pdTRUE);
}

void ad8232_init(ad8232_t *params)
{
    wifi_init_sta();
    uint16_t *ptr;
    adc_continuous_params_t adc_continuous_params;
    adc_continuous_params.adc_gpio = params->gpio;
    adc_continuous_params.atten = ADC_ATTEN_DB_12;
    adc_continuous_params.bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
    adc_continuous_params.conv_frame_size = CONV_FRAME_SIZE;
    adc_continuous_params.max_buf_size = BUFFER_SIZE;
    adc_continuous_params.sample_freq = SAMPLE_FREQ;
    adc_continuous_params.conv_mode = ADC_CONV_SINGLE_UNIT_1;
    adc_continuous_params.cbs.on_conv_done = s_conv_done_cb;

    available_buffers = xQueueCreate(2, sizeof(uint16_t *));
    processing_buffers = xQueueCreate(2, sizeof(uint16_t *));

    if (available_buffers == NULL || processing_buffers == NULL) {
        ESP_LOGE("app_init", "No pudieron crearse las colas de los buffers");
        return;
    }
    
    ptr = ad8232_bufferA;
    xQueueSend(available_buffers, &ptr, 0);
    ptr = ad8232_bufferB;
    xQueueSend(available_buffers, &ptr, 0);

    xTaskCreate(ad8232_conv_done, "ad8232_conv_done", 4096, params, 24, &ad8232_conv_done_handle);
    xTaskCreate(modulation_spectrogram_task, "modulation_spectrogram_task", 4096, params, 23, &ad8232_process_samples_handle);
    xTaskCreate(tcp_client,"tcp_client", 4096, NULL, 22, &tcp_client_handle);

    adc_init(&adc_continuous_params);
    params->adc_handler = adc_continuous_params.adc_handler;
}

void ad8232_conv_done(void *params)
{
    uint8_t conv_frame[CONV_FRAME_SIZE];
    uint8_t cnt = 0;
    uint16_t *buffer;
    uint16_t buffer_idx = 0;
    uint16_t sample = 0;
    ad8232_t *ad8232_data = (ad8232_t *)(params);
    uint32_t out_lenght;

    adc_continuous_data_t parsed_data[CONV_FRAME_SIZE / SOC_ADC_DIGI_RESULT_BYTES];
    
    if (xQueueReceive(available_buffers, &buffer, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE("ad8232", "No se pudo obtener buffer inicial");
        vTaskDelete(NULL);
    }
        
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (!adc_read(ad8232_data->adc_handler, conv_frame, CONV_FRAME_SIZE, &out_lenght)) {
            ESP_LOGI("ad8232_conv_done", "La conversion del frame ha fallado");
            continue;
        }

        esp_err_t parse_ret = adc_continuous_parse_data(ad8232_data->adc_handler, conv_frame, out_lenght, parsed_data, &out_lenght);
        if (parse_ret != ESP_OK) {
            ESP_LOGE("ad8232_conv_done", "Error parseando los datos del ADC");
            continue;
        }
        
        for (uint16_t i = 0; i < out_lenght; i++) {
            sample += parsed_data[i].raw_data;
            cnt++;
            
            if (cnt != 4) {
                continue;
            }

            sample = sample >> 2;
            buffer[buffer_idx++] = sample;
            cnt = 0;
            sample = 0;

            if (buffer_idx < BUFFER_SIZE) {
                continue;
            }

            buffer_idx = 0;
            
            //Esto debería despertar a la tarea de procesamiento
            xQueueSend(processing_buffers, &buffer, portMAX_DELAY);
            xQueueReceive(available_buffers, &buffer, portMAX_DELAY);
        }
    }
}