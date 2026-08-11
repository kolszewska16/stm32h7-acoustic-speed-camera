#ifndef INC_DISPLAY_TASK_H_
#define INC_DISPLAY_TASK_H_

#include "main.h"
#include "lvgl.h"
#include "ili9341.h"

extern SPI_HandleTypeDef hspi3;

void flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);
void vDisplayTask(void *parameter);

#endif /* INC_DISPLAY_TASK_H_ */
