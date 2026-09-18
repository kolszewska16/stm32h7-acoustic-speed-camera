#include "camera_ov5640.h"
#include "ov5640_reg.h"
#include "ov5640_dcmi.h"

Camera_StatusTypeDef Camera_Init(Camera_HandleTypeDef *hcam,
		const OV5640_HandleTypedef *cfg,
		uint8_t *frame_buf, uint32_t frame_size,
		uint32_t resolution, uint32_t pixel_format)
{
	if(hcam == NULL || cfg == NULL || frame_buf == NULL ||
		frame_size == 0 || resolution == 0 || pixel_format == 0)
	{
		return CAMERA_ERROR;
	}

	uint32_t id = 0;

	hcam->cam = *cfg;
	hcam->frame_buf = frame_buf;
	hcam->frame_size = frame_size;
	hcam->is_initialized = 0;

	// I2C and hardware reset
	if(OV5640_BSP_Init(&hcam->sensor, &hcam->cam) != OV5640_OK) {
		return CAMERA_ERROR;
	}

	// OV5640 communication check
	if(OV5640_ReadID(&hcam->sensor, &id) != OV5640_OK) {
		return CAMERA_ERROR;
	}
	if(id != OV5640_ID) {
		return CAMERA_WRONG_ID;
	}

	// OV5640's registers configuration
	if(OV5640_Init(&hcam->sensor, resolution, pixel_format) != OV5640_OK) {
		return CAMERA_ERROR;
	}

	// DVP mode
	if(OV5640_EnableDVPMode(&hcam->sensor) != OV5640_OK) {
		return CAMERA_ERROR;
	}

	// polarities for PCLK, HREF and VSYNC
	if(OV5640_SetPolarities(&hcam->sensor, OV5640_POLARITY_PCLK_HIGH,
		OV5640_POLARITY_HREF_HIGH, OV5640_POLARITY_VSYNC_HIGH) != OV5640_OK)
	{
		return CAMERA_ERROR;
	}

	// DCMI + DMA
	if(Camera_DCMI_Init(frame_buf, frame_size) != CAMERA_DCMI_OK) {
		return CAMERA_ERROR;
	}

	hcam->is_initialized = 1;
	return CAMERA_OK;
}

Camera_StatusTypeDef Camera_CaptureSnapshot(Camera_HandleTypeDef *hcam) {
	if(hcam == NULL || hcam->is_initialized == 0) {
		return CAMERA_NOT_INITIALIZED;
	}

	if(Camera_DCMI_CaptureSnapshot() == CAMERA_DCMI_OK) {
		return CAMERA_OK;
	}
	else {
		return CAMERA_ERROR;
	}
}

uint8_t Camera_IsFrameReady(Camera_HandleTypeDef *hcam) {
	if(hcam == NULL || hcam->is_initialized == 0) {
		return CAMERA_NOT_INITIALIZED;
	}

	return Camera_DCMI_IsFrameReady();
}

Camera_StatusTypeDef Camera_SetBrightness(Camera_HandleTypeDef *hcam, int32_t level) {
	if(hcam == NULL || hcam->is_initialized == 0) {
		return CAMERA_NOT_INITIALIZED;
	}

	if(OV5640_SetBrightness(&hcam->sensor, level) == OV5640_OK) {
		return CAMERA_OK;
	}
	else {
		return CAMERA_ERROR;
	}
}

Camera_StatusTypeDef Camera_SetLightMode(Camera_HandleTypeDef *hcam, uint32_t mode) {
	if(hcam == NULL || hcam->is_initialized == 0) {
		return CAMERA_NOT_INITIALIZED;
	}

	if(OV5640_SetLightMode(&hcam->sensor, mode) == OV5640_OK) {
		return CAMERA_OK;
	}
	else {
		return CAMERA_ERROR;
	}
}

Camera_StatusTypeDef Camera_MirrorFlip(Camera_HandleTypeDef *hcam, uint32_t config) {
	if(hcam == NULL || hcam->is_initialized == 0) {
		return CAMERA_NOT_INITIALIZED;
	}

	if(OV5640_MirrorFlipConfig(&hcam->sensor, config) == OV5640_OK) {
		return CAMERA_OK;
	}
	else {
		return CAMERA_ERROR;
	}
}
