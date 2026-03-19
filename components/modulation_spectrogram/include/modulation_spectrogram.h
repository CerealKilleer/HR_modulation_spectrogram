#ifndef __MODULATION_SPECTROGRAM_H__
#define __MODULATION_SPECTROGRAM_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t buf_size;
    uint32_t fft_n_samples;
    uint32_t fft_win_size;
    uint32_t fft_hop;
    uint32_t fft_n_frames;
    uint32_t fft_freq_bins;
    uint16_t *buffer;
    float *spectrogram;
    float *window;
    float *y;
} spectrogram_t;

typedef struct {
    float *spectrogram;
    float *mod_spectrogram;
    float *window;
    float *y;
    uint32_t spectrogram_n_frames;
    uint32_t spectrogram_freq_bins;
    uint32_t fft_win_size;
    uint32_t fft_hop;
    uint32_t fft_freq_bins;
    uint32_t fft_n_samples;
} modulation_spectrogram_t;

bool fft_init(spectrogram_t *params);
bool fft_spectrogram(spectrogram_t *params);
bool fft_mod_spec_init(modulation_spectrogram_t *params);
bool fft_modulation_spectrogram(modulation_spectrogram_t *params);

#endif
