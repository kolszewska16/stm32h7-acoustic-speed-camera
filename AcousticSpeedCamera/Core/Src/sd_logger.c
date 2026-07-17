#include "sd_logger.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "arm_math.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "ff.h"
#include "os_objects.h"
#include "hardware.h"

#define LOG_FILENAME "log.txt"
#define LOG_FLUSH_EVERY_N 10
#define LOG_QUEUE_TIMEOUT_MS 1000

static FATFS s_fs;
static FIL s_file;

static void sd_log_write_header(void) {
	const char *hdr = "timestamp_ms,power,dbspl\r\n";
	UINT bw;
	f_write(&s_file, hdr, strlen(hdr), &bw);
}

void vSDLogTask(void *argument) {
	(void)argument;
	char line[64];
	FRESULT fr;
	UINT bw;
	uint32_t entries_since_sync = 0;

	if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
		const char *msg = "[INFO] sd logger task start\r\n";
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		osMutexRelease(uartMutex);
	}

	fr = f_mount(&s_fs, "", 1);
	if(fr != FR_OK) {
		if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
			char msg[64];
			snprintf(msg, sizeof(msg), "[ERROR] failed to mount SD card, code: %d\r\n", fr);
			HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			osMutexRelease(uartMutex);
		}

		while(1) {
			vTaskDelay(pdMS_TO_TICKS(1000));
			if(f_mount(&s_fs, "", 1) == FR_OK) {
				if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
					const char *msg = "[INFO] SD card mounted\r\n";
					HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
				}
				break;
			}
		}
	}

	fr = f_open(&s_file, LOG_FILENAME, FA_WRITE | FA_OPEN_APPEND | FA_CREATE_ALWAYS);
	if(fr != FR_OK) {
		if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
			char msg[64];
			snprintf(msg, sizeof(msg), "[ERROR] failed to open file, code: %d\r\n", fr);
			HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			osMutexRelease(uartMutex);
		}

		vTaskDelete(NULL);
		return;
	}

	if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
		const char *msg = "[INFO] opened log file\r\n";
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		osMutexRelease(uartMutex);
	}

	if(f_size(&s_file) == 0) {
		sd_log_write_header();
		f_sync(&s_file);
	}

	LogEntry_t entry;
	while(1) {
		if(xQueueReceive(xLogQueue, &entry, pdMS_TO_TICKS(LOG_QUEUE_TIMEOUT_MS)) == pdTRUE) {
			int len = snprintf(line, sizeof(line), "%lu,%.4f,%.2f\r\n",
					(unsigned long)entry.timestamp_ms,
					entry.power,
					entry.dbspl_avg);

			fr = f_write(&s_file, line, len, &bw);
			if(fr != FR_OK || bw != (UINT)len) {
				f_close(&s_file);
				f_open(&s_file, LOG_FILENAME, FA_WRITE | FA_OPEN_APPEND);
				continue;
			}

			if(++entries_since_sync >= LOG_FLUSH_EVERY_N) {
				f_sync(&s_file);
				entries_since_sync = 0;
			}
		}
		else {
			if(entries_since_sync > 0) {
				f_sync(&s_file);
				entries_since_sync = 0;
			}
		}
	}

	vTaskDelete(NULL);
}
