#ifndef INC_OV5640_DCMI_H_
#define INC_OV5640_DCMI_H_

#include <stdint.h>
#include "main.h"

extern DCMI_HandleTypeDef hdcmi;
extern DMA_HandleTypeDef hdma_dcmi;

typedef enum {
	CAMERA_DCMI_OK = 0,
	CAMERA_DCMI_ERROR = -1,
	CAMERA_DCMI_TIMEOUT = -2
} Camera_DCMI_StatusTypedef;

Camera_DCMI_StatusTypedef Camera_DCMI_Init(uint8_t *frame_buf, uint32_t frame_size_bytes);

Camera_DCMI_StatusTypedef Camera_DCMI_CaptureSnapshot(void);

uint8_t Camera_DCMI_IsFrameReady(void);

#endif /* INC_OV5640_DCMI_H_ */
