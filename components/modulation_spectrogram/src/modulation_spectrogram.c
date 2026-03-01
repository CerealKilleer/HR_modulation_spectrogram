#include "modulation_spectrogram.h"
#include "dsps_fft2r.h"
#include "dsps_wind_hann.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_dsp.h"

__attribute__((aligned(16)))
float window[FFT_N_SAMPLES];

__attribute__((aligned(16)))
float y[FFT_N_SAMPLES * 2];

__attribute__((aligned(16)))
float spectrogram[N_FRAMES][FREQ_BINS];

static inline bool fft_execute(float *y);

bool fft_init(void)
{
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);

    if (ret != ESP_OK) {
        ESP_LOGE("fft_spectrogram", "Error inicializando la tabla de coeficientes de la FFT. Codigo de error: %d\n", ret);
        return false;
    }

    dsps_wind_hann_f32(window, FFT_N_SAMPLES);

    return true;
}

bool fft_spectrogram(uint16_t *data_buffer, size_t buffer_len)
{
    size_t j = 0;
    size_t k = 0;
    size_t frame = 0;
    
    for (size_t i = 0; i <= buffer_len - FFT_N_SAMPLES; i+= HOP) {
        float mean = 0.0;
        j = i;
        k = 0;

        while (k < FFT_N_SAMPLES) {
            mean += data_buffer[j + k];
            k++;
        }
        mean /= FFT_N_SAMPLES;
        k = 0;

        while (k < FFT_N_SAMPLES) {
            y[2 * k] = (data_buffer[j + k] - mean)  * window[k];
            y[2 * k + 1] = 0;
            k++;
        }

        if (!fft_execute(y)) {
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
    //Esto es para ir visualizando en el uart
    printf("START\n");

    for (size_t frame = 0; frame < N_FRAMES; frame++)
    {
        for (size_t bin = 0; bin < FREQ_BINS; bin++)
        {
            printf("%.6f", spectrogram[frame][bin]);

            if (bin < FREQ_BINS - 1)
                printf(" ");
        }
        printf("\n");
    }

    printf("END\n");

    return true;
}

static inline bool fft_execute(float *y)
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