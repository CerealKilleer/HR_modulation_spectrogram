#ifndef __MODULATION_SPECTROGRAM_H__
#define __MODULATION_SPECTROGRAM_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdint.h>

#define SAMPLE_FREQ 1024
#define CONV_FRAME_SIZE 256
#define BUFFER_SIZE 1024 
#define FFT_N_SAMPLES 512
#define FFT_WIN_SIZE 32
#define FFT_HOP_SIZE 8
#define FFT_N_FRAMES (((BUFFER_SIZE - FFT_WIN_SIZE) / FFT_HOP_SIZE) + 1)
#define FREQ_BINS   ((FFT_N_SAMPLES / 2) + 1)

extern uint16_t ad8232_bufferA[BUFFER_SIZE];

extern uint16_t ad8232_bufferB[BUFFER_SIZE];

extern float window[FFT_WIN_SIZE];

typedef struct {
    uint32_t buf_size;
    uint32_t fft_n_samples;
    uint32_t fft_win_size;
    uint32_t fft_hop;
    uint32_t fft_n_frames;
    uint32_t fft_freq_bins;
} modulation_spectrogram_t;

extern QueueHandle_t available_buffers, processing_buffers;

void modulation_spectrogram_task(void *params);

#endif
