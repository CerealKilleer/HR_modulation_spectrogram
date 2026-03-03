#include "modulation_spectrogram.h"
#include "dsps_fft2r.h"
#include "dsps_wind_hann.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "sdkconfig.h"
#include "esp_heap_caps.h"
#include <string.h>

__attribute__((aligned(16)))
uint16_t ad8232_bufferA[BUFFER_SIZE];

__attribute__((aligned(16)))
uint16_t ad8232_bufferB[BUFFER_SIZE];

__attribute__((aligned(16)))
float window[FFT_WIN_SIZE];

__attribute__((aligned(16)))
float y[FFT_N_SAMPLES * 2];

float (*spectrogram)[FREQ_BINS];

float y[FFT_N_SAMPLES * 2];

QueueHandle_t available_buffers, processing_buffers;

static inline bool fft_execute(void);
static bool fft_init(void);
static bool fft_spectrogram(uint16_t *data_buffer, size_t buffer_len);

void modulation_spectrogram_task(void *params)
{
    uint16_t *buffer;
    extern TaskHandle_t tcp_client_handle;

    if (!fft_init()) {
        vTaskDelete(NULL);
    }

    while (1) {
        xQueueReceive(processing_buffers, &buffer, portMAX_DELAY);
        
        fft_spectrogram(buffer, BUFFER_SIZE);
        xTaskNotifyGive(tcp_client_handle);
        xQueueSend(available_buffers, &buffer, portMAX_DELAY);
    }
}

static bool fft_init(void)
{
    spectrogram = heap_caps_malloc((FFT_N_FRAMES) * sizeof(*spectrogram), MALLOC_CAP_SPIRAM);

    if (spectrogram == NULL) {
        ESP_LOGE("fft_spectrogram", "Error reservando PSRAM\n");
        return false;
    }

    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);

    if (ret != ESP_OK) {
        ESP_LOGE("fft_spectrogram", "Error inicializando la tabla de coeficientes de la FFT. Codigo de error: %d\n", ret);
        free(spectrogram);
        return false;
    }

    dsps_wind_hann_f32(window, FFT_WIN_SIZE);
    return true;
}

static bool fft_spectrogram(uint16_t *data_buffer, size_t buffer_len)
{
    size_t j = 0;
    size_t k = 0;
    size_t frame = 0;

    for (size_t i = 0; i <= buffer_len - FFT_WIN_SIZE; i+= FFT_HOP_SIZE) {
        float mean = 0.0;
        j = i;
        k = 0;

        memset(y, 0, sizeof(y));

        while (k < FFT_WIN_SIZE) {
            mean += data_buffer[j + k];
            k++;
        }

        mean /= FFT_WIN_SIZE;
        k = 0;

        while (k < FFT_WIN_SIZE) {
            y[2 * k] = (data_buffer[j + k] - mean)  * window[k];
            y[2 * k + 1] = 0;
            k++;
        }

        if (!fft_execute()) {
            ESP_LOGE("fft_spectrogram", "Error calculando la FFT");
            return false;
        }

        for (size_t fft_dot = 0; fft_dot < FREQ_BINS; fft_dot++) {
            float y_r = y[2*fft_dot] / FFT_N_SAMPLES;
            float y_c = y[2*fft_dot + 1] / FFT_N_SAMPLES;
            spectrogram[frame][fft_dot] = y_r * y_r + y_c * y_c;
        }

        frame++;
    }
    ESP_LOGI("fft_spectrogram", "Espectrograma generado\n");
    return true;
}

static inline bool fft_execute(void)
{
    esp_err_t ret;
    ret = dsps_fft2r_fc32(y, FFT_N_SAMPLES);

    if (ret != ESP_OK) {
        ESP_LOGE("fft_execute - dsps_fft2r_fc32", "Error calculando la FFT");
        dsps_fft2r_deinit_fc32();
        return false;
    }

    ret = dsps_bit_rev_fc32(y, FFT_N_SAMPLES);

    if (ret != ESP_OK) {
        ESP_LOGE("fft_execute - dsps_bit_rev_fc32", "Error calculando la FFT");
        dsps_fft2r_deinit_fc32();
        return false;
    }

    ret = dsps_cplx2reC_fc32(y, FFT_N_SAMPLES);

    if (ret != ESP_OK) {
        ESP_LOGE("fft_execute - dsps_cplx2reC_fc32", "Error calculando la FFT");
        dsps_fft2r_deinit_fc32();
        return false;
    }

    return true;
}