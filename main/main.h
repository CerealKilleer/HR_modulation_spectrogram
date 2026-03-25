#ifndef __MAIN_H__
#define __MAIN_H__
#define AD8232_GPIO 4

//STFFT parameters
#define SAMPLE_FREQ 1024
#define SIGNAL_SAMPLE_FREQ 256
#define ADC_OVERSAMPLING 4
#define CONV_FRAME_SIZE 256
#define BUFFER_SIZE 1024 
#define FFT_N_SAMPLES 512
#define FFT_WIN_SIZE 32
#define FFT_HOP_SIZE 8
#define FFT_N_FRAMES (((BUFFER_SIZE - FFT_WIN_SIZE) / FFT_HOP_SIZE) + 1)
#define FREQ_BINS   ((FFT_N_SAMPLES / 2) + 1)

//Modulation spectrogram params
#define FFT_MOD_SPEC_N_SAMPLES 512
#define FFT_MOD_SPEC_WIN_SIZE 128
#define FFT_MOD_SPEC_HOP_SIZE 32
#define FFT_MOD_SPEC_FREQ_BINS ((FFT_MOD_SPEC_N_SAMPLES / 2) + 1)

//HR freq limits
#define MAX_SIGNAL_FREQ 20
#define MIN_SIGNAL_FREQ 5
#define MIN_MOD_SPEC_FREQ 0.8
#define MAX_MOD_SPEC_FREQ 3.3
#endif