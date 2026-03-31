#include "audio_processor.h"

arm_rfft_fast_instance_f32 fft_handler;

int32_t dmabuff_L[BUFF_SIZE];
int32_t dmabuff_R[BUFF_SIZE];

float32_t hanning_window[SAMPLES];
float32_t hanning_window_energy = 0.0f;
float32_t a_weighting_table[SAMPLES];

float32_t fft_inputL[SAMPLES];
float32_t fft_inputR[SAMPLES];
float32_t fft_outputL[SAMPLES];
float32_t fft_outputR[SAMPLES];
float32_t fft_magnitudesL[SAMPLES / 2];
float32_t fft_magnitudesR[SAMPLES / 2];
