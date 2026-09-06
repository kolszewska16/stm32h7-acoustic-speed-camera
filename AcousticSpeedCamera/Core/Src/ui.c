/**
 * @file ui.c
 *
 * @brief LVGL-based user interface for the acoustic speed camera display.
 *
 * @details See ui.h for the public API documentation. This file contains
 * 			the LVGL display driver glue for the ILI9341 panel (flush,
 * 			callback and buffer setup) and the measurement screen layout
 * 			and update logic. The panel is driven over SPI using DMA: the
 * 			flush callback only starts the transfer, and the buffer is
 * 			released back to LVGL from HAL_SPI_TxCplCallback() in main.c
 * 			once the transfer completes.
 */

#include "ui.h"
#include <stdint.h>

/** @brief Label showing the live dBA reading at the center of the screen. */
static lv_obj_t *value_label;

/** @brief Label showing the scrolling status message at the bottom of the screen. */
static lv_obj_t *status_label;

/** @brief Label showing the peak (max) dBA value recorded so far. */
static lv_obj_t *max_val_label;

/** @brief Label showing the current NORM/ALARM noise status. */
static lv_obj_t *norm_status_label;

/**
 * @brief Display currently being flushed via DMA.
 *
 * @details Set by flush_cb() just before starting the SPI DMA transfer, and
 * 			read back by HAL_SPI_TxCpltCallback() (in main.c) to know which
 * 			LVGL display to notify once the transfers completes. Declared
 * 			volatile since it is written from the main context and read
 * 			from an ISR context.
 */
volatile lv_display_t *active_disp = NULL;

void flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
	if(display == NULL || area == NULL || px_map == NULL) {
		return;
	}

	ILI9341_SetWindow(&lcd, area->x1, area->y1, area->x2, area->y2);
	active_disp = display;
	uint32_t size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1) * 2;

	HAL_GPIO_WritePin(lcd.dc_port, lcd.dc_pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(lcd.cs_port, lcd.cs_pin, GPIO_PIN_RESET);

	HAL_SPI_Transmit_DMA(lcd.hspi, px_map, size);
}

void display_init(ILI9341_HandleTypeDef *lcd) {
	if(lcd == NULL) {
		return;
	}

	lv_init();
	HAL_GPIO_WritePin(lcd->bl_port, lcd->bl_pin, GPIO_PIN_SET);
	ILI9341_Init(lcd);

	lv_tick_set_cb(HAL_GetTick);
	lv_display_t *display1 = lv_display_create(HORIZONTAL_RESOLUTION, VERICAL_RESOLUTION);

	// double buffering
	static uint8_t buf1[HORIZONTAL_RESOLUTION * VERICAL_RESOLUTION / 10 * BYTES_PER_PIXEL];
	static uint8_t buf2[HORIZONTAL_RESOLUTION * VERICAL_RESOLUTION / 10 * BYTES_PER_PIXEL];

	lv_display_set_buffers(display1, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_flush_cb(display1, flush_cb);
}

void measurement_screen_init(void) {
	// root screen
	lv_obj_t *scr = lv_screen_active();
	lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D0D0D), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

	// title bar
	lv_obj_t *title_label = lv_label_create(scr);
	lv_label_set_text(title_label, "TRAFFIC NOISE MONITOR");
	lv_obj_set_style_text_color(title_label, lv_color_hex(0x7F8C8D), LV_PART_MAIN);
	lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, LV_PART_MAIN);
	lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 15);

	// top-left: current noise status (NORM / ALARM), updated in update_norm_status_label()
	norm_status_label = lv_label_create(scr);
	lv_label_set_text(norm_status_label, "NORM");
	lv_obj_set_style_text_color(norm_status_label, lv_color_hex(0x2ECC71), LV_PART_MAIN);
	lv_obj_set_style_text_font(norm_status_label, &lv_font_montserrat_14, LV_PART_MAIN);
	lv_obj_align(norm_status_label, LV_ALIGN_TOP_LEFT, 15, 45);

	// top-right: peak dBA value recorded so far, updated in update_max_val_label()
	max_val_label = lv_label_create(scr);
	lv_label_set_text(max_val_label, "MAX: -- dBA");
	lv_obj_set_style_text_color(max_val_label, lv_color_hex(0x95A5A6), LV_PART_MAIN);
	lv_obj_set_style_text_font(max_val_label, &lv_font_montserrat_14, LV_PART_MAIN);
	lv_obj_align(max_val_label, LV_ALIGN_TOP_RIGHT, -15, 45);

	// center: live dBA reading, updated in update_measurement_value()
	value_label = lv_label_create(scr);
	lv_label_set_text(value_label, "---");
	lv_obj_set_style_text_color(value_label, lv_color_hex(0xECF0F1), LV_PART_MAIN);
	lv_obj_set_style_text_font(value_label, &lv_font_montserrat_40, LV_PART_MAIN);
	lv_obj_set_width(value_label, 105);
	lv_obj_set_style_align(value_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
	lv_obj_align(value_label, LV_ALIGN_CENTER, -20, 0);

	// unit label
	lv_obj_t *unit_label = lv_label_create(scr);
	lv_label_set_text(unit_label, "dBA");
	lv_obj_set_style_text_color(unit_label, lv_color_hex(0x00D2D3), LV_PART_MAIN);
	lv_obj_set_style_text_font(unit_label, &lv_font_montserrat_20, LV_PART_MAIN);
	lv_obj_align_to(unit_label, value_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, -5);

	// bottom status bar container
	lv_obj_t *status_bar = lv_obj_create(scr);
	lv_obj_set_size(status_bar, 320, 30);
	lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x1A1A1A), 0);
	lv_obj_set_style_border_color(status_bar, lv_color_hex(0x333333), 0);
	lv_obj_set_style_border_width(status_bar, 1, 0);
	lv_obj_set_style_pad_all(status_bar, 0, 0);
	lv_obj_set_scrollbar_mode(status_bar, LV_SCROLLBAR_MODE_OFF);
	lv_obj_align(status_bar, LV_ALIGN_BOTTOM_MID, 0, 0);

	// scrolling status text inside the status bar, updated in update_status_bar()
	status_label = lv_label_create(status_bar);
	lv_label_set_text(status_label, "[INFO] system ready to use");
	lv_obj_set_style_text_color(status_label, lv_color_hex(0xB2BEC3), LV_PART_MAIN);
	lv_obj_set_width(status_label, 240);
	lv_label_set_long_mode(status_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, LV_PART_MAIN);
	lv_obj_align(status_label, LV_ALIGN_LEFT_MID, 10, 0);
}

void update_measurement_value(float32_t value) {
	int whole = (int32_t)value;
	int frac = (int32_t)((value - whole) * 10.0f);
	if(frac < 0) {
		frac = -frac;
	}

	char buf[64];
	lv_snprintf(buf, sizeof(buf), "%d.%d", (int)whole, (int)frac);
	lv_label_set_text(value_label, buf);
}

void update_status_bar(const char *msg) {
	if(msg == NULL) {
		return;
	}

	lv_label_set_text(status_label, msg);
}

void update_max_val_label(float32_t value) {
	int whole = (int32_t)value;
	int frac = (int32_t)((value - whole) * 10.0f);
	if(frac < 0) {
		frac = -frac;
	}

	char buf[32];
	lv_snprintf(buf, sizeof(buf), "MAX: %d.%d dB", (int)whole, (int)frac);
	lv_label_set_text(max_val_label, buf);
}

void update_norm_status_label(float32_t value) {
	if(value > 70.0f) {
		lv_label_set_text(norm_status_label, "ALARM");
		lv_obj_set_style_text_color(norm_status_label, lv_color_hex(0xE74C3C), LV_PART_MAIN);
	}
	else {
		lv_label_set_text(norm_status_label, "NORM");
		lv_obj_set_style_text_color(norm_status_label, lv_color_hex(0x2ECC71), LV_PART_MAIN);
	}
}
