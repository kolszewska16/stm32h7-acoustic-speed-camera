#include <sd_task.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "arm_math.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "ff.h"
#include "rtc.h"
#include "os_objects.h"
#include "sd_spi.h"
#include "logger.h"
#include "sd_utils.h"

#define IMAGE_QUEUE_TIMEOUT_MS 1000

static FATFS s_fs;

static void update_ui_sd_status(void) {
	uint64_t free_b;
	uint64_t total_b;

	if(SD_GetFreeSpace("", &free_b, &total_b) != FR_OK || total_b == 0) {
		return;
	}

	uint8_t percent_free = (uint8_t)((free_b * 100ULL) / total_b);
	xQueueOverwrite(xUiSdStatusQueue, &percent_free);
}

static void update_ui_status_bar(const char *msg) {
	UiStatusMsg_t status_msg;
	strncpy(status_msg.text, msg, UI_STATUS_MSG_MAX_LEN - 1);
	status_msg.text[UI_STATUS_MSG_MAX_LEN - 1] = '\0';

	xQueueSend(xUiStatusQueue, &status_msg, 0);
}

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
	update_ui_sd_status();

	ImageSaveRequest_t req;
	while(1) {
		if(xQueueReceive(xImageQueue, &req, pdMS_TO_TICKS(IMAGE_QUEUE_TIMEOUT_MS)) == pdTRUE) {
			uint64_t free_bytes;
			SD_SpaceStatus_t sd_space = SD_CheckSpaceBeforeWrite("", req.len, &free_bytes);
			if(sd_space == SD_SPACE_FULL) {
				LOG_ERROR("not enough space to save image (%lu bytes needed)", req.len);
				update_ui_sd_status();
				update_ui_status_bar("[ERROR] SD card full");

				if(cameraTaskHandle != NULL) {
					xTaskNotifyGive((TaskHandle_t)cameraTaskHandle);
				}

				continue;
			}

			RTC_TimeTypeDef time;
			RTC_DateTypeDef date;
			HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

			char filename[13];
			snprintf(filename, sizeof(filename), "%02u%02u%02u%02u.jpg",
				date.Date, time.Hours, time.Minutes, time.Seconds);

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
				update_ui_sd_status();
			}

			if(cameraTaskHandle != NULL) {
				xTaskNotifyGive((TaskHandle_t)cameraTaskHandle);
			}
		}
	}

	vTaskDelete(NULL);
}
