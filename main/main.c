#include <stdio.h>
#include "main.h"
#include "ad8232.h"
#include "modulation_spectrogram.h"
#include "wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "portmacro.h"
#include "spec_mqtt_client.h"
#include "tcp_client.h"

static const char *TAG = "app";

TaskHandle_t tcp_client_handle;
TaskHandle_t mqtt_client_handle;
TaskHandle_t app_ad8232_handle;
TaskHandle_t app_do_spectrogram_handle;
TaskHandle_t app_do_mod_spectrogram_handle;
TaskHandle_t wifi_handle;
TaskHandle_t app_calc_hr_handle;
adc_continuous_handle_t adc_handle;
QueueHandle_t available_buffer;
QueueHandle_t processing_buffer;
QueueHandle_t hr_buffer;

__attribute__((aligned(16)))
uint16_t ad8232_bufferA[BUFFER_SIZE];

__attribute__((aligned(16)))
uint16_t ad8232_bufferB[BUFFER_SIZE];

__attribute__((aligned(16)))
float window[FFT_WIN_SIZE];

__attribute__((aligned(16)))
float mod_spec_window[FFT_MOD_SPEC_WIN_SIZE];

__attribute__((aligned(16)))
float y[FFT_N_SAMPLES * 2];

__attribute__((aligned(16)))
float y_mod[FFT_MOD_SPEC_N_SAMPLES * 2];

float *spectrogram;
float *mod_spectrogram;

static bool IRAM_ATTR conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    vTaskNotifyGiveFromISR(app_ad8232_handle, &mustYield);
    return (mustYield == pdTRUE);
}

static bool app_init_ad8232(void)
{
    ad8232_adc_config_t ad8232_cfg = {
        .adc_continuous_cfg.adc_gpio = AD8232_GPIO,
        .adc_continuous_cfg.atten = ADC_ATTEN_DB_12,
        .adc_continuous_cfg.bit_width = SOC_ADC_DIGI_MAX_BITWIDTH,
        .adc_continuous_cfg.conv_frame_size = CONV_FRAME_SIZE,
        .adc_continuous_cfg.max_buf_size = BUFFER_SIZE,
        .adc_continuous_cfg.sample_freq = SAMPLE_FREQ,
        .adc_continuous_cfg.conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .adc_continuous_cfg.cbs.on_conv_done = conv_done_cb
    };

    if (!ad8232_init(&ad8232_cfg)) {
        return false;
    }

    adc_handle = ad8232_cfg.adc_continuous_cfg.adc_handler;
    
    return true;
}

static bool app_init_queues(void)
{
    available_buffer = xQueueCreate(2, sizeof(uint16_t *));
    uint16_t *buffer_ptr;

    if (available_buffer == NULL) {
        ESP_LOGE(TAG, "Error creando la cola de buffers disponibles");
        return false;
    }

    processing_buffer = xQueueCreate(2, sizeof(uint16_t *));
    if (processing_buffer == NULL) {
        vQueueDelete(available_buffer);
        ESP_LOGE(TAG, "Error creando la cola de buffers listos para procesar");
        return false;
    }

    hr_buffer = xQueueCreate(1, sizeof(float));

    if (hr_buffer == NULL) {
        vQueueDelete(available_buffer);
        vQueueDelete(processing_buffer);
        ESP_LOGE(TAG, "Error creando la cola de ritmo cardiaco");
        return false;
    }

    buffer_ptr = ad8232_bufferA;
    if (xQueueSend(available_buffer, (void *)&buffer_ptr, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Error inicializando la cola de buffers disponibles.");
        return false;
    }

    buffer_ptr = ad8232_bufferB;
    if (xQueueSend(available_buffer, (void *)&buffer_ptr, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Error inicializando la cola de buffers disponibles.");
        return false;
    }


    return true;
}

void app_do_spectrogram(void *params)
{
    uint16_t *buffer;
    spectrogram_t spec_params = {
        .buf_size = BUFFER_SIZE,
        .fft_n_samples = FFT_N_SAMPLES,
        .fft_win_size = FFT_WIN_SIZE,
        .fft_hop = FFT_HOP_SIZE,
        .fft_n_frames = FFT_N_FRAMES,
        .fft_freq_bins = FREQ_BINS,
        .window = window,
        .y = y
    };

    if (!fft_init(&spec_params)) {
        ESP_LOGE(TAG, "No se pudo crear la fft");
        vTaskDelete(NULL);
    }

    spectrogram = spec_params.spectrogram;

    for (;;) {
        if (xQueueReceive(processing_buffer, (void *)&buffer, portMAX_DELAY) != pdTRUE) {
            ESP_LOGE(TAG, "No se pudo obtener un nuevo buffer para procesar");
        }

        spec_params.buffer = buffer;
        fft_spectrogram(&spec_params);

        xTaskNotifyGive(app_do_mod_spectrogram_handle);

        if (xQueueSend(available_buffer, (void *)&buffer, portMAX_DELAY) != pdTRUE) {
            ESP_LOGE(TAG, "No se pudo enviar el buffer a procesamiento");
        }
    }
}

void app_do_mod_spectrogram(void *params)
{
    modulation_spectrogram_t mod_spec = {
        .spectrogram = spectrogram,
        .window = mod_spec_window,
        .y = y_mod,
        .spectrogram_n_frames = FFT_N_FRAMES,
        .spectrogram_freq_bins = FREQ_BINS,
        .fft_win_size = FFT_MOD_SPEC_WIN_SIZE,
        .fft_hop = FFT_MOD_SPEC_HOP_SIZE,
        .fft_freq_bins = FFT_MOD_SPEC_FREQ_BINS,
        .fft_n_samples = FFT_MOD_SPEC_N_SAMPLES
    };

    if (!fft_mod_spec_init(&mod_spec)) {
        ESP_LOGE(TAG, "No se pudo crear la fft");
        vTaskDelete(NULL);
    }

    mod_spectrogram = mod_spec.mod_spectrogram;

    while(true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        fft_modulation_spectrogram(&mod_spec);
        xTaskNotifyGive(app_calc_hr_handle);
    }
}

void app_calc_hr(void *params) {

    float spectrogram_freq_res = ((float) SIGNAL_SAMPLE_FREQ) / FFT_N_SAMPLES;
    float mod_spec_res = ((float) SIGNAL_SAMPLE_FREQ) / FFT_HOP_SIZE / FFT_MOD_SPEC_N_SAMPLES;

    hr_t hr_params = {
        .mod_spectrogram = mod_spectrogram,
        .mod_freq_res = mod_spec_res,
        .mod_freq_low_sample = MIN_MOD_SPEC_FREQ / mod_spec_res,
        .mod_freq_high_sample = MAX_MOD_SPEC_FREQ / mod_spec_res + 1,
        .signal_freq_high_sample = (MAX_SIGNAL_FREQ / spectrogram_freq_res + 1),
        .signal_freq_low_sample = (MIN_SIGNAL_FREQ / spectrogram_freq_res)
    };

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        float hr = get_hr(&hr_params);
        xQueueSend(hr_buffer, (void *)&hr, portMAX_DELAY);
    }
}

void app_ad8232_task(void *params) 
{
    
    int ret = -1;
    uint16_t *buffer;
    uint8_t conv_frame[CONV_FRAME_SIZE];
    adc_continuous_data_t parsed_data[CONV_FRAME_SIZE / SOC_ADC_DIGI_RESULT_BYTES];

    ad8232_adc_samples_t adc_samples = {
        .conv_frame = conv_frame,
        .parsed_data = parsed_data,
        .buffer_size = BUFFER_SIZE,
        .conv_frame_size = CONV_FRAME_SIZE,
        .oversampling = ADC_OVERSAMPLING
    };

    adc_samples.adc_handle = adc_handle;
    
    if (xQueueReceive(available_buffer, (void *)&buffer, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "No se pudo obtener el primer buffer para almacenar las muestras");
        vTaskDelete(NULL);
    }

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        adc_samples.buffer = buffer;

        ret = ad8232_get_samples(&adc_samples);
        
        if (ret != 0) {
            continue;
        }

        ESP_LOGI(TAG, "Buffer generado");

        if (xQueueSend(processing_buffer, (void *)&buffer, portMAX_DELAY) != pdTRUE) {
            ESP_LOGE(TAG, "No se pudo enviar el buffer a procesamiento");
        }

        if (xQueueReceive(available_buffer, (void *)&buffer, portMAX_DELAY) != pdTRUE) {
            ESP_LOGE(TAG, "No se pudo obtener buffer para almacenar las muestras");
        }
    }
}

void app_mqtt_client(void *params)
{
    mqtt_client_init();
    float hr;

    for (;;) {
        xQueueReceive(hr_buffer, (void *)&hr, portMAX_DELAY);
        mqtt_client_publish_hr(hr);
        //xTaskNotifyGive(tcp_client_handle);
    }
}

void app_main(void)
{
    wifi_init_sta();

    if (!app_init_ad8232()) {
        ESP_LOGE(TAG, "Error al inicializar el módulo AD8232");
        vTaskDelete(NULL);
    }

    if (!app_init_queues()) {
        vTaskDelete(NULL);
    }

    if (xTaskCreate(app_ad8232_task, "app_ad8232_task", 4096, NULL, 24, &app_ad8232_handle) != pdTRUE) {
        ESP_LOGE(TAG, "Error al crear la tarea para el AD8232");
        vTaskDelete(NULL);
    }
    
    if (xTaskCreate(app_do_spectrogram, "app_do_spectrogram", 4096, NULL, 23, &app_do_spectrogram_handle) != pdTRUE) {
        ESP_LOGE(TAG, "Error al crear la tarea para generar los espectrogramas");
        vTaskDelete(NULL);
    }

    if (xTaskCreate(app_do_mod_spectrogram, "app_do_mod_spectrogram", 4096, NULL, 23, &app_do_mod_spectrogram_handle) != pdTRUE) {
        ESP_LOGE(TAG, "Error al crear la tarea para generar los espectrogramas");
        vTaskDelete(NULL);
    }

    if (xTaskCreate(app_calc_hr, "app_calc_hr", 4096, NULL, 23, &app_calc_hr_handle) != pdTRUE) {
        ESP_LOGE(TAG, "Error al crear la tarea para calcular el ritmo cardiaco");
        vTaskDelete(NULL);
    }

    if (xTaskCreate(app_mqtt_client,"mqtt_client", 4096, NULL, 22, &mqtt_client_handle) != pdTRUE) {
        ESP_LOGE(TAG, "Error al crear la tarea del cliente mqtt");
        vTaskDelete(NULL);
    }
    
    xTaskCreate(tcp_client,"tcp_client", 4096, NULL, 22, &tcp_client_handle);
    ad8232_adc_start(adc_handle);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
