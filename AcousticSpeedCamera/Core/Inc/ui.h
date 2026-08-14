#ifndef INC_UI_H_
#define INC_UI_H_

#include "arm_math.h"
#include "lvgl.h"
#include "ili9341.h"

#define HORIZONTAL_RESOLUTION 320
#define VERICAL_RESOLUTION 240
#define BYTES_PER_PIXEL LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)

extern ILI9341_HandleTypeDef lcd;

void flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);
void display_init(ILI9341_HandleTypeDef *lcd);

void measurement_screen_init(void);

void update_measurement_value(float32_t value);
void update_status_bar(const char *msg);
void update_max_val_label(float32_t value);
void update_norm_status_label(float32_t);

#endif /* INC_UI_H_ */
