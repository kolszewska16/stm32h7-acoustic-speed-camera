#include "camera_task.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ov5640_dcmi.h"
#include "camera_ov5640.h"
#include "os_objects.h"
#include "logger.h"

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240
#define FRAME_BPP 2

#define INIT_MAX_ATTEMPTS 3

#define SNAPSHOT_MAX_ATTEMPTS 3
#define SNAPSHOT_TIMEOUT_MS 1000
#define SNAPSHOT_POLL_INTERVAL_MS 20
#define SNAPSHOT_RETRY_DELAY_MS 20

static Camera_HandleTypeDef s_hcam;
static uint8_t frame_buf[FRAME_WIDTH * FRAME_HEIGHT * FRAME_BPP];

void vCameraTask(void *parameter) {
	LOG_INFO("camera task start");

	OV5640_HandleTypedef cfg = {
		.hi2c = &hi2c1,
		.rst_port = GPIOG,
		.rst_pin = GPIO_PIN_4,
		.pwdn_port = GPIOG,
		.pwdn_pin = GPIO_PIN_5,
	};

	osDelay(500);

	Camera_StatusTypeDef status = CAMERA_ERROR;
	for(int attempt = 0; attempt < INIT_MAX_ATTEMPTS && status != CAMERA_OK; attempt++) {
		LOG_INFO("attempt=%d", attempt);
		status = Camera_Init(&s_hcam, &cfg, frame_buf, sizeof(frame_buf),
				OV5640_R320x240, OV5640_RGB565);

		if(status != CAMERA_OK) {
			osDelay(200);
		}
	}

	if(status != CAMERA_OK) {
		LOG_ERROR("camera initialization failed");
		vTaskDelete(NULL);
		return;
	}

	LOG_INFO("camera initialization completed");
	camera_ready = true;

	while(1) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

/*		if(Camera_CaptureSnapshot(&s_hcam) != CAMERA_OK) {
			LOG_ERROR("failed to capture the snapshot");
		}*/

		uint8_t frame_ok = 0;

		for(int attempt = 0; attempt < SNAPSHOT_MAX_ATTEMPTS && !frame_ok; attempt++) {
			if(Camera_CaptureSnapshot(&s_hcam) != CAMERA_OK) {
				LOG_ERROR("failed to capture the snapshot (attempt=%d)", attempt);
				osDelay(pdMS_TO_TICKS(SNAPSHOT_RETRY_DELAY_MS));
				continue;
			}

			uint32_t waited_ms = 0;
			while(waited_ms < SNAPSHOT_TIMEOUT_MS) {
				if(Camera_IsFrameReady(&s_hcam)) {
					frame_ok = 1;
					break;
				}

				osDelay(pdMS_TO_TICKS(SNAPSHOT_POLL_INTERVAL_MS));
				waited_ms += SNAPSHOT_POLL_INTERVAL_MS;
			}

			if(!frame_ok) {
				LOG_ERROR("failed to receive the snapshot (attempt=%d, dcmi_state=%d)",
					attempt, Camera_DCMI_GetState());
			}
		}

		if(frame_ok) {
			LOG_INFO("snapshot received");
		}
		else {
			LOG_ERROR("snapshot permanently failed after %d attempts", SNAPSHOT_MAX_ATTEMPTS);
		}
	}

	vTaskDelete(NULL);
}
