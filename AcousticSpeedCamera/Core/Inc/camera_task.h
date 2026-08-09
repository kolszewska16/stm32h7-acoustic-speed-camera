#ifndef INC_CAMERA_TASK_H_
#define INC_CAMERA_TASK_H_

#include "main.h"

extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;

void vCameraTask(void *parameter);

#endif /* INC_CAMERA_TASK_H_ */
