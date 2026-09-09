#include "audio_processor.h"
#include <stdio.h>
#include <string.h>
#include "stm32h7xx_nucleo.h"
#include "dfsdm.h"
#include "logger.h"

arm_rfft_fast_instance_f32 fft_handler;

int32_t dmabuff_L[BUFF_SIZE];
int32_t dmabuff_R[BUFF_SIZE];

float32_t hanning_window[SAMPLES];
float32_t hanning_window_energy = 0.0f;
float32_t a_weighting_table[SAMPLES / 2];

float32_t fft_inputL[SAMPLES];
float32_t fft_inputR[SAMPLES];
float32_t fft_outputL[SAMPLES];
float32_t fft_outputR[SAMPLES];
float32_t fft_magnitudesL[SAMPLES / 2];
float32_t fft_magnitudesR[SAMPLES / 2];

HAL_StatusTypeDef audio_hardware_start(void) {
	HAL_StatusTypeDef state_L = HAL_DFSDM_FilterRegularStart_DMA(&hdfsdm1_filter0,
			dmabuff_L, BUFF_SIZE);
	HAL_StatusTypeDef state_R = HAL_DFSDM_FilterRegularStart_DMA(&hdfsdm1_filter1,
			dmabuff_R, BUFF_SIZE);

	if(state_L != HAL_OK || state_R != HAL_OK) {
		return HAL_ERROR;
	}

	return HAL_OK;
}

void init_hanning_window(float32_t *buf, float32_t *energy_out) {
	float32_t energy = 0.0f;

	for(int i = 0; i < SAMPLES; i++) {
		buf[i] = 0.5f * (1 - cosf((2 * PI * i) / (SAMPLES - 1)));
		energy += buf[i] * buf[i];
	}

	*energy_out = energy;

	LOG_INFO("window energy: %.2f", energy);
}

void init_a_weighting_table(float32_t *buf) {
	buf[0] = 0.0f;
	for(int i = 1; i < SAMPLES / 2; i++) {
		float32_t f = i * FS / SAMPLES;
		float32_t f2 = f * f;
		float32_t numerator = powf(12194.0f, 2.0f) * f2 * f2;
		float32_t denominator = (f2 + powf(20.6f, 2.0f)) * sqrtf((f2 + powf(107.7f, 2.0f)) *
				(f2 + powf(737.9f, 2.0f))) * (f2 + powf(12194.0f, 2.0f));
		float32_t Ra = numerator / denominator;
		float32_t Af = 20.0f * log10f(Ra) + 2.0f;
		buf[i] = powf(10.0f, Af / 10.0f);
	}
}

void audio_dsp_init(void) {
	init_hanning_window(hanning_window, &hanning_window_energy);
	init_a_weighting_table(a_weighting_table);
	arm_rfft_fast_init_f32(&fft_handler, FFT_SIZE);
}

void process_input_buffers(int32_t *bufL, int32_t *bufR, uint32_t offset) {
	if(bufL == NULL || bufR == NULL) {
		return;
	}

	for(int i = 0; i < SAMPLES; i++) {
		fft_inputL[i] = ((float32_t)(bufL[i + offset]) / 32768.0f);
		fft_inputR[i] = ((float32_t)(bufR[i + offset]) / 32768.0f);
	}

	// removing DC offset
	float32_t meanL = 0.0f;
	float32_t meanR = 0.0f;

	arm_mean_f32(fft_inputL, SAMPLES, &meanL);
	arm_mean_f32(fft_inputR, SAMPLES, &meanR);

	for(int i = 0; i < SAMPLES; i++) {
		fft_inputL[i] -= meanL;
		fft_inputR[i] -= meanR;
	}
}

float32_t calculate_channel_power(float32_t *in, float32_t *out, float32_t *mag) {
	if(in == NULL || out == NULL || mag == NULL) {
		return 0;
	}

	// windowing
	arm_mult_f32(in, hanning_window, in, SAMPLES);

	// FFT
	arm_rfft_fast_f32(&fft_handler, in, out, 0);

	// magnitude & dBA
	arm_cmplx_mag_f32(out, mag, SAMPLES / 2);

	float32_t total_power = 0.0f;
	for(int i = 0; i < SAMPLES / 2; i++) {
		mag[i] /= (float32_t)SAMPLES;
		total_power += mag[i] * mag[i] * a_weighting_table[i];
	}

	return total_power / hanning_window_energy;
}

float32_t process_audio_frame(int32_t *bufL, int32_t *bufR, uint32_t offset) {
	if(bufL == NULL || bufR == NULL) {
		return 0;
	}

	process_input_buffers(bufL, bufR, offset);
	float32_t powerL = calculate_channel_power(fft_inputL, fft_outputL, fft_magnitudesL);
	float32_t powerR = calculate_channel_power(fft_inputR, fft_outputR, fft_magnitudesR);

	float32_t power_avg = (powerL + powerR) / 2.0f;
	return 10 * log10f(power_avg + 1e-30f) + CALIBRATION_OFFSET;
}
