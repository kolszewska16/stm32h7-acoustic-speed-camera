#include "ov5640_dcmi.h"
#include "dcmi.h"
#include "logger.h"

static uint32_t s_frame_addr;
static uint32_t s_frame_size_words;
static volatile uint8_t s_frame_ready;
static volatile uint32_t s_dcmi_last_error;

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi_handle) {
	s_frame_ready = 1;
}

void HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi_handle) {
	s_dcmi_last_error = HAL_DCMI_GetError(hdcmi_handle);
	LOG_ERROR("DCMI error: 0x0%8lX", s_dcmi_last_error);
	s_frame_ready = 0;
}

Camera_DCMI_StatusTypedef Camera_DCMI_Init(uint8_t *frame_buf, uint32_t frame_size_bytes) {
	if(frame_buf == NULL || frame_size_bytes == 0) {
		return CAMERA_DCMI_ERROR;
	}

	s_frame_addr = (uint32_t)frame_buf;
	s_frame_size_words = frame_size_bytes / 4;
	s_frame_ready = 0;

	return 0;
}

Camera_DCMI_StatusTypedef Camera_DCMI_CaptureSnapshot(void) {
	s_frame_ready = 0;

	if(HAL_DCMI_GetState(&hdcmi) != HAL_DCMI_STATE_READY) {
		HAL_DCMI_Stop(&hdcmi);
	}

	if(HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT,
			s_frame_addr, s_frame_size_words) != HAL_OK)
	{
		LOG_ERROR("DCMI state=%d, error=0x%8lX",
			HAL_DCMI_GetState(&hdcmi), HAL_DCMI_GetError(&hdcmi));
		return CAMERA_DCMI_ERROR;
	}

	return CAMERA_DCMI_OK;
}

uint8_t Camera_DCMI_IsFrameReady(void) {
	uint8_t ready = s_frame_ready;
	s_frame_ready = 0;

	return ready;
}

int32_t Camera_DCMI_GetState(void) {
	return HAL_DCMI_GetState(&hdcmi);
}

uint32_t Camera_DCMI_GetLastError(void) {
	return s_dcmi_last_error;
}
