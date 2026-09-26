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
#include "sd_spi.h"
#include "logger.h"

#define IMAGE_QUEUE_TIMEOUT_MS 1000

static FATFS s_fs;

void vSDLogTask(void *argument) {
	(void)argument;
	FRESULT fr;

	LOG_INFO("sd logger task start");

	fr = f_mount(&s_fs, "", 1);
	if(fr != FR_OK) {
		LOG_ERROR("failed to mount SD card, code: %d", fr);

		while(1) {
			vTaskDelay(pdMS_TO_TICKS(1000));
			if(f_mount(&s_fs, "", 1) == FR_OK) {
				LOG_INFO("SD card mounted");
				break;
			}
		}
	}

	sd_ready = true;

	ImageSaveRequest_t req;
	while(1) {
		if(xQueueReceive(xImageQueue, &req, pdMS_TO_TICKS(IMAGE_QUEUE_TIMEOUT_MS)) == pdTRUE) {
			char filename[13];
			snprintf(filename, sizeof(filename), "%08lu.jpg", (unsigned long)req.timestamp_ms);

			FIL img_file;
			FRESULT img_fr = f_open(&img_file, filename, FA_WRITE | FA_CREATE_ALWAYS);

			if(img_fr != FR_OK) {
				LOG_ERROR("failed to save %s, code: %d", filename, img_fr);
			}
			else {
				UINT bw;
				f_write(&img_file, req.data, req.len, &bw);
				f_close(&img_file);
				LOG_INFO("saved %s (%lu bytes)", filename, req.len);
			}

			if(cameraTaskHandle != NULL) {
				xTaskNotifyGive((TaskHandle_t)cameraTaskHandle);
			}
		}
	}

	vTaskDelete(NULL);
}
