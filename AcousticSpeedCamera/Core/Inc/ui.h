/**
 * @file ui.h
 *
 * @brief LVGL-based user interface for the acoustic speed camera display.
 *
 * @details Provides the display driver glue between LVGL and ILI9341
 * 			TFT controller (via SPI DMA), and the UI layout/update functions
 * 			for the noise measurement screen.
 */

#ifndef INC_UI_H_
#define INC_UI_H_

#include "arm_math.h"
#include "lvgl.h"
#include "ili9341.h"

/** @brief Display horizontal resolution, in pixels. */
#define HORIZONTAL_RESOLUTION 320

/** @brief Display vertical resolution, in pixels. */
#define VERICAL_RESOLUTION 240

/** @brief Size, in bytes, of a single pixel in LVGL's RGB565 color format. */
#define BYTES_PER_PIXEL LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)

/**
 * @brief 	Global ILI9341 driver handler, shared with the rest of the
 * 			application.
 */
extern ILI9341_HandleTypeDef lcd;

/**
 * @brief 	LVGL display flush callback: sends a rendered pixel buffer to
 * 			the ILI9341 over SPI using DMA.
 *
 * @details Registered with LVGL via lv_display_set_flush_cb(). Called by
 * 			LVGL whenever a rendered area needs to be pushed to the panel.
 * 			The transfer is non-blocking: this function only starts the
 * 			DMA transfer and returns immediately. LVGL is notified that the
 * 			buffer is free again (lv_display_flush_ready()) from
 * 			HAL_SPI_TxCpltCallback() once the DMA transfer has completed.
 *
 * @param[in] display	Pointer to the LVGL display object being flushed.
 * @param[in] area		Pointer to the rectangular area (in panel coordinates)
 * 						that px_map covers.
 * @param[in] px_map	Pointer to the rendered pixel buffer, RGB585 color
 * 						format, as laid out by LVGL for the given area.
 *
 * @return None
 *
 * @note 	LV_COLOR_16_SWAP must be enabled in lv_conf.h: LVGL stores
 * 			RGB565 pixels little-endian in px_map, while the ILI9341
 * 			expects big-endian byte order over SPI. Without the swap, pixel
 * 			colors arrive corrupted even though the transfer itself succeeds.
 */
void flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);

/**
 * @brief Initializes the ILI9341 panel and registers it as and LVGL display.
 *
 * @details Initializes LVGL, turns on the panel backlight, initializes the
 * 			ILI9341 controller, creates the LVGL display object, and
 * 			configures double buffering with partial render mode so that
 * 			LVGL can render into one buffer while the previous one is still
 * 			being transferred by DMA.
 *
 * @param[in] lcd	Pointer to the ILI9341 handle describing the panel's GPIO
 * 					and SPI configuration. Must not be NULL.
 *
 * @return None
 */
void display_init(ILI9341_HandleTypeDef *lcd);

/**
 * @brief Builds the noise measurement screen layout (labels, status bar).
 *
 * @details Creates and positions all LVGL objects for the main measurement
 * 			screen: title, norm/alarm status, peak value, live dBA reading,
 * 			and a scrolling status bar. Must be called once, after
 * 			display_init(), before any of the update_*() functions below
 * 			are used.
 *
 * @return None
 */
void measurement_screen_init(void);

/**
 * @brief Updates the live dBA reading shown at the center of the screen.
 *
 * @param[in] value Current noise level, in dBA.
 *
 * @return None
 */
void update_measurement_value(float32_t value);

/**
 * @brief Updates the scrolling status bar text at the bottom of the screen.
 *
 * @input[in] msg	Null-terminated status message to display. If NULL, the
 * 					function returns without modifying the label.s
 *
 * @return None
 */
void update_status_bar(const char *msg);

/**
 * @brief Updates the peak (maximum) dBA value shown in the top-right corner.
 *
 * @param[in] value	Peak noise level recorded so far, in dBA.
 *
 * @return None
 */
void update_max_val_label(float32_t value);

/**
 * @brief Updates the NORM/ALARM status label based on the noise threshold.
 *
 * @details Sets the label text and color to red "ALARM" if value exceeds
 * 			70dBA, or green "NORM" otherwise.
 *
 * @param[in] value	Current noise level, in dBA, used for the threshold check.
 *
 * @return None
 */
void update_norm_status_label(float32_t value);

/**
 * @brief
 *
 * @details
 *
 * @param[in] dBA_avg
 * @param[in] dBA_max
 *
 * @return None
 */
void update_ui(float32_t dBA_avg, float32_t dBA_max);

#endif /* INC_UI_H_ */
