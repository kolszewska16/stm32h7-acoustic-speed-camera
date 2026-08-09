#ifndef INC_AUDIO_PROCESSOR_H_
#define INC_AUDIO_PROCESSOR_H_

#include "arm_math.h"
#include "defines.h"

extern arm_rfft_fast_instance_f32 fft_handler;

extern int32_t dmabuff_L[BUFF_SIZE];
extern int32_t dmabuff_R[BUFF_SIZE];

extern float32_t hanning_window[SAMPLES];
extern float32_t a_weighting_table[SAMPLES / 2];

extern float32_t fft_inputL[SAMPLES];
extern float32_t fft_inputR[SAMPLES];
extern float32_t fft_outputL[SAMPLES];
extern float32_t fft_outputR[SAMPLES];
extern float32_t fft_magnitudesL[SAMPLES / 2];
extern float32_t fft_magnitudesR[SAMPLES / 2];

void init_hanning_window(float32_t *buf, float32_t *energy_out);
void init_a_weighting_table(float32_t *buf);

void vAudioTask(void *argument);

#endif /* INC_AUDIO_PROCESSOR_H_ */
