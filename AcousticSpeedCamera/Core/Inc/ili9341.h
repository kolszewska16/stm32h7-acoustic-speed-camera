/**
 * @file ili9341.h
 *
 * @brief Driver for the ILI9341 TFT LCD controller (SPI mode).
 *
 * @details Provides low-level command/data transaction primitives and
 * 			panel initialization/addressing functions for the ILI9341,
 * 			implemented against the ILI9341 datasheet command set. The
 * 			driver targets 4-wire SPI (with separate D/C line) and
 * 			RGB565 pixel data. Bulk pixel transfers (e.g. for LVGL flush
 * 			operations) are expected to be issued by the caller directly
 * 			via HAL_SPI_Transmit_DMA() after ILI9341_SetWindow(), rather
 * 			than through this driver - see ui.c.
 */

#ifndef INC_ILI9341_H_
#define INC_ILI9341_H_

#include <stdint.h>
#include "main.h"

/**
 * @brief Handle describing one ILI9341 panel instance.
 *
 * @details Groups the SPI peripheral handle together with the GPIO
 * 			pins used for chip select (CS), data/command selection (DC),
 * 			hardware reset (RST), and backlight control (BL).
 */
typedef struct {
    SPI_HandleTypeDef *hspi;	/**< SPI handle used to communicate with the panel. */

    GPIO_TypeDef *cs_port;		/**< GPIO port for the chip select (CS) pin. */
    uint16_t cs_pin;			/**< GPIO pin for the chip select (CS) line. */

    GPIO_TypeDef *dc_port;		/**< GPIO port for the data/command select (DC) pin. */
    uint16_t dc_pin;			/**< GPIO pin for the data/command select (DC) line. */

    GPIO_TypeDef *rst_port;		/**< GPIO port for the hardware reset (RST) pin. */
    uint16_t rst_pin;			/**< GPIO pin for the hardware reset (RST) line. */

    GPIO_TypeDef *bl_port;		/**< GPIO port for the backlight (BL) pin. */
    uint16_t bl_pin;			/**< GPIO pin for the backlight (BL) line. */
} ILI9341_HandleTypeDef;

/**
 * @brief Status codes returned by the higher-level ILI9341 driver functions.
 */
typedef enum {
    LCD_OK,					/**< Operation completed successfully. */
    LCD_ERROR,				/**< Generic error (e.g. NULL handle or invalid argument. */
    LCD_ERROR_WRITE_CMD,	/**< A command byte transaction failed. */
    LCD_ERROR_WRITE_DATA,	/**< A single data byte transaction failed. */
    LCD_ERROR_SEND_DATA,	/**< A 16-bit data word transaction failed. */
} lcdStatus_t;

/**
 * @brief Sends a single command byte to the ILI9341.
 *
 * @details Pulls DC low to select command mode, asserts CS, transmits the
 * 			command byte over SPI (blocking), then deasserts CS.
 *
 * @param[in] lcd Pointer to the panel handle. Must not be NULL.
 * @param[in] cmd Command byte to send, as defined by the ILI9341 command set.
 *
 * @return	HAL_OK on success, HAL_ERROR if lcd is NULL or the SPI
 * 			transactions fails.
 */
HAL_StatusTypeDef ILI9341_WriteCommand(ILI9341_HandleTypeDef *lcd, uint8_t cmd);

/**
 * @brief Sends a single 8-bit data byte to the ILI9341.
 *
 * @details Pulls DC high to select data mode, asserts CS, transmits the
 * 			data byte over SPI (blocking), then deasserts CS. Used to send
 * 			command parameters (e.g. pixel format, addressing argument).
 *
 * @param[in] lcd	Pointer to the panel handle. Must not be NULL.
 * @param[in] data	Data byte to send.
 *
 * @return	HAL_OK on success, HAL_ERROR if lcd is NULL or the SPI
 * 			transaction fails.
 */
HAL_StatusTypeDef ILI9341_WriteData(ILI9341_HandleTypeDef *lcd, uint8_t data);

/**
 * @brief	Sends a single 16-bit data word (e.g. one RGB565 pixel) to the
 * 			ILI9341.
 *
 * @details Pulls DC high to select data mode, asserts CS, transmits the
 * 			16-bit value as two bytes, most significant byte first
 * 			(big-endian), matching the byte order expected by the ILI9341
 * 			over SPI. Intended for single-pixel or low-throughput writes;
 * 			bulk transfers should use DMA directly instead (see ui.c).
 *
 * @param[in] lcd	Pointer to the panel handle. Must not be NULL.
 * @param[in] data	16-bit value to send.
 *
 * @return	HAL_OK on success, HAL_ERROR if lcd is NULL or the SPI
 * 			transaction fails.
 */
HAL_StatusTypeDef ILI9341_SendData(ILI9341_HandleTypeDef *lcd, uint16_t data);

/**
 * @brief	Performs hardware reset and initialization sequence for the
 * 			ILI9341 panel.
 *
 * @details Toggles RST line, issues a software reset, then configures
 * 			the pixel format (16 bits/pixel, RGB565), memory access
 * 			control (scan direction/orientation), exits sleep mode, and
 * 			turns the display on. Required delays between steps follow the
 * 			ILI9341 datasheet's power-on sequence.
 *
 * @param[in] lcd Pointer to the panel handle. Must not be NULL.
 *
 * @return	LCD_OK on success, LCD_ERROR if lcd is NULL,
 * 			LCD_ERROR_WRITE_CMD or LCD_ERROR_WRITE_DATA if any steps of the
 * 			sequence fails.
 */
lcdStatus_t ILI9341_Init(ILI9341_HandleTypeDef *lcd);

/**
 * @brief	Sets the active drawing window (column/page address range) on
 * 			the panel.
 *
 * @details Issues the column address set (0x2A) and page address set (0x2B)
 * 			commands with the given bounds, followed by the memory write
 * 			(0x2C) command. After this call, the panel is ready to receive
 * 			pixel data for specified rectangular area, written in
 * 			row-major order.
 *
 * @param[in] lcd	Pointer to the panel handle. Must not be NULL.
 * @param[in] x1	Left column of the window, in pixels (0-based).
 * @param[in] y1	Top row of the window, in pixels (0-based).
 * @param[in] x2	Right column of the window, in pixels (inclusive).
 * @param[in] y2	Bottom row of the window, in pixels (inclusive).
 *
 * @return	LCD_OK on success, LCD_ERROR if lcd is NULL,
 * 			LCD_ERROR_WRITE_CMD or LCD_ERROR_WRITE_DATA if any steps of the
 * 			sequence fails.
 */
lcdStatus_t ILI9341_SetWindow(ILI9341_HandleTypeDef *lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**
 * @brief Fills the entire panel with a single solid color.
 *
 * @details Sets the drawing window to the full panel resolution
 * 			(320x240) and blocking-transmits the given color, one pixel
 * 			at a time, over SPI. Intended as a hardware bring-up/sanity
 * 			test, not for normal rendering (LVGL's flush-cb/DMA path in
 * 			ui.c should be used instead for actual UI updates).
 *
 * @param[in] lcd	Pointer to the panel handle. Must not be NULL.
 * @param[in] color	Color to fill the screen with, in RGB565 format.
 *
 * @return	LCD_OK on success, LCD_ERROR if lcd is NULL, the window
 * 			could not be set, or any pixel transmission fails.
 */
lcdStatus_t ILI9341_Test(ILI9341_HandleTypeDef *lcd, uint16_t color);

#endif /* INC_ILI9341_H_ */
