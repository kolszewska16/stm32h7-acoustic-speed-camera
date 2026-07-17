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

#include "fatfs.h"
#include "hardware.h"
#include "os_objects.h"
#include "audio_processor.h"
#include "sd_logger.h"

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
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void vAudioTask(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

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
	uartMutex = osMutexNew(&uartMutex_attr);

  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
	s_spi_dma_sem = osSemaphoreNew(1, 0, NULL);

  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
	xLogQueue = xQueueCreate(32, sizeof(LogEntry_t));

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  audioTaskHandle = osThreadNew(vAudioTask, NULL, &audioTask_attr);
  sdLoggerTaskHandle = osThreadNew(vSDLogTask, NULL, &sdLoggerTask_attr);

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
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

	uint32_t count_lost_entries = 0;
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
			fft_inputL[i] = (float32_t)(dmabuff_L[i + offset] / 131072.0f);
			fft_inputR[i] = (float32_t)(dmabuff_R[i + offset] / 131072.0f);
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

		float32_t power_avg = (total_powerL + total_powerR) / 2.0f;
		float32_t dBA_avg = 10.0f * log10f(power_avg + 1e-30f) + MIC_DBFS_TO_DBSPL;

/*		float32_t dBA_L = 0.0f;
		float32_t dBA_R = 0.0f;
		dBA_L = 10.0f * log10f(total_powerL + 1e-30f) + MIC_DBFS_TO_DBSPL;
		dBA_R = 10.0f * log10f(total_powerR + 1e-30f) + MIC_DBFS_TO_DBSPL;*/

		LogEntry_t entry;
		entry.timestamp_ms = HAL_GetTick();
		entry.power = power_avg;
		entry.dbspl_avg = dBA_avg;

		if(xQueueSend(xLogQueue, &entry, pdMS_TO_TICKS(10)) != pdTRUE) {
			count_lost_entries++;
			if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
				char msg[64];
				snprintf(msg, sizeof(msg), "[WARNING] lost entries: %lu\r\n", count_lost_entries);
				HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
				osMutexRelease(uartMutex);
			}
		}
	}

	vTaskDelete(NULL);
}

/* USER CODE END Application */

