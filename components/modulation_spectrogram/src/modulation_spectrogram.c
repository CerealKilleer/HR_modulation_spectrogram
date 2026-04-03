#include "modulation_spectrogram.h"
#include "dsps_fft2r.h"
#include "dsps_wind_hann.h"
#include "dsps_wind_blackman.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <stddef.h>
#include <string.h>
#include <math.h>

static bool fft_execute(float *y, size_t fft_n_samples);

bool fft_init(spectrogram_t *params)
{
    params->spectrogram = heap_caps_malloc((params->fft_n_frames) * sizeof(float) * (params->fft_freq_bins), MALLOC_CAP_SPIRAM);

    if (params->spectrogram == NULL) {
        ESP_LOGE("fft_spectrogram", "Error reservando PSRAM\n");
        return false;
    }

    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);

    if (ret != ESP_OK) {
        ESP_LOGE("fft_spectrogram", "Error inicializando la tabla de coeficientes de la FFT. Codigo de error: %d\n", ret);
        free(params->spectrogram);
        return false;
    }

    dsps_wind_hann_f32(params->window, params->fft_win_size);

    return true;
}

bool fft_mod_spec_init(modulation_spectrogram_t *params)
{
    params->mod_spectrogram = heap_caps_malloc((params->spectrogram_freq_bins) * sizeof(float) * (params->fft_freq_bins), MALLOC_CAP_SPIRAM);

    if (params->mod_spectrogram == NULL) {
        ESP_LOGE("fft_mod_spec_init", "Error reservando PSRAM\n");
        return false;
    }

    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);

    if (ret != ESP_OK) {
        ESP_LOGE("fft_mod_spec_init", "Error inicializando la tabla de coeficientes de la FFT. Codigo de error: %d\n", ret);
        free(params->mod_spectrogram);
        return false;
    }

    dsps_wind_blackman_f32(params->window, params->fft_win_size);
    return true;
}

bool fft_modulation_spectrogram(modulation_spectrogram_t *params)
{
    size_t spectrogram_n_frames = params->spectrogram_n_frames;
    size_t spectrogram_freq_bins = params->spectrogram_freq_bins; 
    uint32_t fft_n_samples = params->fft_n_samples;
    uint32_t fft_freq_bins = params->fft_freq_bins;
    float *spectrogram = params->spectrogram;
    float *mod_spec = params->mod_spectrogram;

    for (size_t i = 0; i < spectrogram_freq_bins; i++) {
        memset(params->y, 0, 2 * fft_n_samples * sizeof(float));
        float mean = 0.0;
        size_t k = 0;

        for (size_t j = 0; j < spectrogram_n_frames; j++) {
            float sample = spectrogram[i + j * spectrogram_freq_bins];
            mean += sample;
            params->y[2*k] = sample;
            k++;
        }

        mean /= spectrogram_n_frames;

        for (size_t j = 0; j < k; j++) {
            params->y[2*j] -= mean;
            params->y[2*j] *= params->window[j];
        }

        if (!fft_execute(params->y, fft_n_samples)) {
            ESP_LOGE("fft_spectrogram", "Error calculando la FFT");
            return false;
        }

        for (size_t fft_dot = 0; fft_dot < fft_freq_bins; fft_dot++) {
            float y_r = (params->y)[2*fft_dot];
            float y_c = (params->y)[2*fft_dot + 1];
            float energy = (y_r * y_r + y_c * y_c);

            if (fft_dot != 0 && fft_dot != (fft_freq_bins - 1)) {
                energy *= 2;
            }

            mod_spec[i * fft_freq_bins + fft_dot] =  energy / fft_n_samples;
        }
    }

    ESP_LOGI("fft_modulation_spectrogram", "Espectrograma de modulación, generado");
    return true;
}

bool fft_spectrogram(spectrogram_t *params)
{
    size_t j = 0;
    size_t k = 0;
    size_t frame = 0;
    const size_t fft_n_samples = params->fft_n_samples;
    const size_t fft_win_size = params->fft_win_size;
    const size_t buffer_size = params->buf_size;
    const size_t fft_hop = params->fft_hop;
    const size_t fft_freq_bins = params->fft_freq_bins;

    for (size_t i = 0; i <= buffer_size - fft_win_size; i+= fft_hop) {
        float mean = 0.0;
        j = i;
        k = 0;

        memset(params->y, 0, 2 * fft_n_samples * sizeof(float));

        while (k < fft_win_size) {
            mean += (params->buffer)[j + k];
            k++;
        }

        mean /= fft_win_size;
        k = 0;

        while (k < fft_win_size) {
            (params->y)[2 * k] = (params->buffer[j + k] - mean)  * params->window[k];
            (params->y)[2 * k + 1] = 0;
            k++;
        }

        if (!fft_execute(params->y, fft_n_samples)) {
            ESP_LOGE("fft_spectrogram", "Error calculando la FFT");
            return false;
        }


        for (size_t fft_dot = 0; fft_dot < fft_freq_bins; fft_dot++) {
            float y_r = (params->y)[2*fft_dot];
            float y_c = (params->y)[2*fft_dot + 1];
            float energy = (y_r * y_r + y_c * y_c);

            if (fft_dot != 0 && fft_dot != (fft_freq_bins - 1)) {
                energy *= 2;
            }

            params->spectrogram[frame * fft_freq_bins + fft_dot] =  energy / fft_n_samples;
        }

        frame++;
    }

    ESP_LOGI("fft_spectrogram", "Espectrograma generado\n");
    return true;
}

static bool fft_execute(float *y, size_t fft_n_samples)
{
    esp_err_t ret;
    ret = dsps_fft2r_fc32(y, fft_n_samples);

    if (ret != ESP_OK) {
        ESP_LOGE("fft_execute - dsps_fft2r_fc32", "Error calculando la FFT");
        dsps_fft2r_deinit_fc32();
        return false;
    }

    ret = dsps_bit_rev_fc32(y, fft_n_samples);

    if (ret != ESP_OK) {
        ESP_LOGE("fft_execute - dsps_bit_rev_fc32", "Error calculando la FFT");
        dsps_fft2r_deinit_fc32();
        return false;
    }

    ret = dsps_cplx2reC_fc32(y, fft_n_samples);

    if (ret != ESP_OK) {
        ESP_LOGE("fft_execute - dsps_cplx2reC_fc32", "Error calculando la FFT");
        dsps_fft2r_deinit_fc32();
        return false;
    }

    return true;
}

float get_hr(hr_t *hr_params)
{
    float *mod_spec = hr_params->mod_spectrogram;
    size_t f_max = hr_params->signal_freq_high_sample;
    size_t f_min = hr_params->signal_freq_low_sample;
    size_t fm_low = hr_params->mod_freq_low_sample;
    size_t fm_high = hr_params->mod_freq_high_sample;

    float max_energy = -1.0f;
    size_t best_fm_idx = 0;

    for (size_t j = fm_low; j <= fm_high; j++) {

        float energy_sum = 0.0f;

        for (size_t i = f_min; i < f_max; i++) {

            float sample = mod_spec[i * 257 + j];            
            energy_sum += sample;
        }

        energy_sum /= (f_max - f_min);

        //float fm = hr_params->mod_freq_res * j;
        //energy_sum /= (1.0f + fm);

        if (energy_sum > max_energy) {
            max_energy = energy_sum;
            best_fm_idx = j;
        }
    }

    float hr = 60.0f * (hr_params->mod_freq_res * best_fm_idx);

    return hr;
}