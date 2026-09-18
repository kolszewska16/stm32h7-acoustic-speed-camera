#include "camera_task.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "camera_ov5640.h"
#include "logger.h"

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240
#define FRAME_BPP 2

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

	osDelay(100);
	if(Camera_Init(&s_hcam, &cfg, frame_buf, sizeof(frame_buf),
		OV5640_R320x240, OV5640_RGB565) != CAMERA_OK)
	{
		LOG_ERROR("camera initialization failed");
		vTaskDelete(NULL);
		return;
	}

	LOG_INFO("initialization completed");

	if(Camera_CaptureSnapshot(&s_hcam) != CAMERA_OK) {
		LOG_ERROR("failed to capture the snapshot");
		vTaskDelete(NULL);
		return;
	}

	uint8_t frame_ok = 0;
	uint32_t waited_ms = 0;
	const uint32_t timeout_ms = 1000;
	const uint32_t poll_interval_ms = 20;

	while(waited_ms < timeout_ms) {
		if(Camera_IsFrameReady(&s_hcam)) {
			frame_ok = 1;
			break;
		}

		osDelay(pdMS_TO_TICKS(poll_interval_ms));
		waited_ms += poll_interval_ms;
	}

	if(frame_ok) {
		LOG_INFO("snapshot received");
	}
	else {
		LOG_ERROR("failed to receive the snapshot");
	}

	vTaskDelete(NULL);
}
