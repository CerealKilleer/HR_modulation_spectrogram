#ifndef __MODULATION_SPECTROGRAM_H__
#define __MODULATION_SPECTROGRAM_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_BUF_SIZE 1024 
#define FFT_N_SAMPLES 32 
#define N_FRAMES 125
#define HOP 8
#define FREQ_BINS   ((FFT_N_SAMPLES / 2) + 1) 

bool fft_init(void);
bool fft_spectrogram(uint16_t *data_buffer, size_t buffer_len);
#endif
