/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <string.h>

#include "hardware.h"
#include "os_objects.h"
#include "audio_processor.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void vAudioTask(void *argument);

/* USER CODE END FunctionPrototypes */

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  audioTaskHandle = osThreadNew(vAudioTask, NULL, &audioTask_attr);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_DFSDM_FilterRegConvHalfCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm) {
	if(hdfsdm == &hdfsdm1_filter0) {
		osThreadFlagsSet(audioTaskHandle, 0x01);
	}

	if(hdfsdm == &hdfsdm1_filter1) {
		osThreadFlagsSet(audioTaskHandle, 0x01);
	}
}

void HAL_DFSDM_FilterRegConvCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm) {
	if(hdfsdm == &hdfsdm1_filter0) {
		osThreadFlagsSet(audioTaskHandle, 0x02);
	}

	if(hdfsdm == &hdfsdm1_filter1) {
		osThreadFlagsSet(audioTaskHandle, 0x02);
	}
}

void vAudioTask(void *parameter) {
	const char *msg = "audio task start\r\n";
	HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

	HAL_StatusTypeDef state_L = HAL_DFSDM_FilterRegularStart_DMA(&hdfsdm1_filter0, dmabuff_L, BUFF_SIZE);
	HAL_StatusTypeDef state_R = HAL_DFSDM_FilterRegularStart_DMA(&hdfsdm1_filter1, dmabuff_R, BUFF_SIZE);

	if(state_L != HAL_OK || state_R != HAL_OK) {
		const char *msg = "DFSDM start error\r\n";
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
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
		for(int i = 0; i < SAMPLES; i++) {
			fft_inputL[i] = (float32_t)(dmabuff_L[i + offset] >> 8);
			fft_inputR[i] = (float32_t)(dmabuff_R[i + offset] >> 8);
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
			fft_magnitudesL[i] /= SAMPLES;
			fft_magnitudesR[i] /= SAMPLES;
		}

		float32_t total_powerL = 0.0f;
		float32_t total_powerR = 0.0f;
		for(int i = 1; i < SAMPLES / 2; i++) {
			total_powerL += fft_magnitudesL[i] * fft_magnitudesL[i] * a_weighting_table[i];
			total_powerR += fft_magnitudesR[i] * fft_magnitudesR[i] * a_weighting_table[i];
		}

		total_powerL /= hanning_window_energy;
		total_powerR /= hanning_window_energy;

		float32_t dBA_L = 0.0f;
		float32_t dBA_R = 0.0f;
		dBA_L = 10.0f * log10f(total_powerL);
		dBA_R = 10.0f * log10f(total_powerR);

		char msg[1024];
		sprintf(msg, "dBA L: %.2f, dBA R: %.2f\r\n", dBA_L, dBA_R);
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), 10);
	}
	vTaskDelete(NULL);
}

/* USER CODE END Application */

