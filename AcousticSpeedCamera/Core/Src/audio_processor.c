#include "audio_processor.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "stm32h7xx_nucleo.h"
#include "dfsdm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "os_objects.h"

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

void init_hanning_window(float32_t *buf, float32_t *energy_out) {
	float32_t energy = 0.0f;

	for(int i = 0; i < SAMPLES; i++) {
		buf[i] = 0.5f * (1 - cosf((2 * PI * i) / (SAMPLES - 1)));
		energy += buf[i] * buf[i];
	}

	*energy_out = energy;

	if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
		char msg[64];
		snprintf(msg, sizeof(msg), "[INFO] window energy: %.2f\r\n", energy);
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		osMutexRelease(uartMutex);
	}
}

void init_a_weighting_table(float32_t *buf) {
	for(int i = 1; i < SAMPLES / 2; i++) {
		float32_t f = i * FS / SAMPLES;
		float32_t f2 = f * f;
		float32_t numerator = powf(12194.0f, 2.0f) * f2 * f2;
		float32_t denominator = (f2 + powf(20.6f, 2.0f)) * sqrtf((f2 + powf(107.7f, 2.0f)) * (f2 + powf(737.9f, 2.0f))) * (f2 + powf(12194.0f, 2.0f));
		float32_t Ra = numerator / denominator;
		float32_t Af = 20.0f * log10f(Ra) + 2.0f;
		buf[i] = powf(10.0f, Af / 10.0f);
	}
}

void vAudioTask(void *parameter) {
	if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
		const char *msg = "[INFO] audio task start\r\n";
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		osMutexRelease(uartMutex);
	}

	HAL_StatusTypeDef state_L = HAL_DFSDM_FilterRegularStart_DMA(&hdfsdm1_filter0, dmabuff_L, BUFF_SIZE);
	HAL_StatusTypeDef state_R = HAL_DFSDM_FilterRegularStart_DMA(&hdfsdm1_filter1, dmabuff_R, BUFF_SIZE);

	if(state_L != HAL_OK || state_R != HAL_OK) {
		if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
			const char *msg = "[ERROR] DFSDM: initialization failed\r\n";
			HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			osMutexRelease(uartMutex);
		}
	}

	init_hanning_window(hanning_window, &hanning_window_energy);
	init_a_weighting_table(a_weighting_table);

	if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
		const char *msg = "[INFO] initialization completed\r\n";
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		osMutexRelease(uartMutex);
	}

	arm_rfft_fast_init_f32(&fft_handler, FFT_SIZE);

	while(1) {
		uint32_t flags = osThreadFlagsWait(0x03, osFlagsWaitAny, osWaitForever);
		uint32_t part_idx = -1;
		if(flags & 0x01) {
			part_idx = 0;
		}
		else if(flags & 0x02) {
			part_idx = 1;
		}

		uint32_t offset = part_idx * SAMPLES;

		int32_t minL = INT32_MAX;
		int32_t maxL = INT32_MIN;
		for(int i = 0; i < SAMPLES; i++) {
			int32_t v = dmabuff_L[i + offset];
			if(v < minL) {
				minL = v;
			}
			if(v > maxL) {
				maxL = v;
			}
		}

		for(int i = 0; i < SAMPLES; i++) {
			fft_inputL[i] = ((float32_t)(dmabuff_L[i + offset]) / 32768.0f);
			fft_inputR[i] = ((float32_t)(dmabuff_R[i + offset]) / 32768.0f);
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

		// windowing
		arm_mult_f32(fft_inputL, hanning_window, fft_inputL, SAMPLES);
		arm_mult_f32(fft_inputR, hanning_window, fft_inputR, SAMPLES);

		// FFT
		arm_rfft_fast_f32(&fft_handler, fft_inputL, fft_outputL, 0);
		arm_rfft_fast_f32(&fft_handler, fft_inputR, fft_outputR, 0);

		// magnitude & dBA
		arm_cmplx_mag_f32(fft_outputL, fft_magnitudesL, SAMPLES / 2);
		arm_cmplx_mag_f32(fft_outputR, fft_magnitudesR, SAMPLES / 2);

		for(int i = 0; i < SAMPLES / 2; i++) {
			fft_magnitudesL[i] /= (float32_t)SAMPLES;
			fft_magnitudesR[i] /= (float32_t)SAMPLES;
		}

		float32_t total_powerL = 0.0f;
		float32_t total_powerR = 0.0f;
		for(int i = 1; i < SAMPLES / 2; i++) {
			total_powerL += fft_magnitudesL[i] * fft_magnitudesL[i] * a_weighting_table[i];
			total_powerR += fft_magnitudesR[i] * fft_magnitudesR[i] * a_weighting_table[i];
		}

		total_powerL /= hanning_window_energy;
		total_powerR /= hanning_window_energy;

		float32_t power_avg = (total_powerL + total_powerR) / 2.0f;
		float32_t dBA_avg = 10.0f * log10f(power_avg + 1e-30f) + CALIBRATION_OFFSET;

/*		float32_t dBA_L = 0.0f;
		float32_t dBA_R = 0.0f;
		dBA_L = 10.0f * log10f(total_powerL + 1e-30f) + MIC_DBFS_TO_DBSPL;
		dBA_R = 10.0f * log10f(total_powerR + 1e-30f) + MIC_DBFS_TO_DBSPL;*/

		if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
/*			char msg[64];
			snprintf(msg, sizeof(msg), "power: %.2f dbspl: %.2f\r\n", power_avg, dBA_avg);*/
			char msg[128];
			snprintf(msg, sizeof(msg), "Power (raw): %.4e | SPL: %.2f dBA\r\n", power_avg, dBA_avg);
			HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			osMutexRelease(uartMutex);
		}

	}

	vTaskDelete(NULL);
}
