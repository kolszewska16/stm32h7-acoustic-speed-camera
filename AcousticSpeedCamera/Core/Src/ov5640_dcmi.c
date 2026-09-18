#include "ov5640_dcmi.h"

static uint32_t s_frame_addr;
static uint32_t s_frame_size_words;
static volatile uint8_t s_frame_ready;

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi_handle) {
	s_frame_ready = 1;
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

	if(HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT,
			s_frame_addr, s_frame_size_words) != HAL_OK)
	{
		return CAMERA_DCMI_ERROR;
	}

	return CAMERA_DCMI_OK;
}

uint8_t Camera_DCMI_IsFrameReady(void) {
	uint8_t ready = s_frame_ready;
	s_frame_ready = 0;

	return ready;
}
