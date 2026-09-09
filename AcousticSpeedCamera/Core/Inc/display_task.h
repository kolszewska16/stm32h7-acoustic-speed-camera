#ifndef INC_DISPLAY_TASK_H_
#define INC_DISPLAY_TASK_H_

#include "main.h"
#include "lvgl.h"
#include "ili9341.h"

extern SPI_HandleTypeDef hspi3;

void vDisplayTask(void *parameter);

#endif /* INC_DISPLAY_TASK_H_ */
