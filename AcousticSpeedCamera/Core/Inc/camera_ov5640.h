#ifndef INC_CAMERA_OV5640_H_
#define INC_CAMERA_OV5640_H_

#include <stdint.h>
#include "ov5640.h"
#include "ov5640_bsp.h"

typedef enum {
	CAMERA_OK = 0,
	CAMERA_ERROR = -1,
	CAMERA_NOT_INITIALIZED = -2,
	CAMERA_WRONG_ID = -3
} Camera_StatusTypeDef;

typedef struct {
	OV5640_Object_t sensor;
	OV5640_HandleTypedef cam;
	uint8_t *frame_buf;
	uint32_t frame_size;
	uint8_t is_initialized;
} Camera_HandleTypeDef;

Camera_StatusTypeDef Camera_Init(Camera_HandleTypeDef *hcam,
		const OV5640_HandleTypedef *cfg,
		uint8_t *frame_buf, uint32_t frame_size,
		uint32_t resolution, uint32_t pixel_format);
Camera_StatusTypeDef Camera_CaptureSnapshot(Camera_HandleTypeDef *hcam);
uint8_t Camera_IsFrameReady(Camera_HandleTypeDef *hcam);

Camera_StatusTypeDef Camera_SetBrightness(Camera_HandleTypeDef *hcam, int32_t level);
Camera_StatusTypeDef Camera_SetLightMode(Camera_HandleTypeDef *hcam, uint32_t mode);
Camera_StatusTypeDef Camera_MirrorFlip(Camera_HandleTypeDef *hcam, uint32_t config);

#endif /* INC_CAMERA_OV5640_H_ */
