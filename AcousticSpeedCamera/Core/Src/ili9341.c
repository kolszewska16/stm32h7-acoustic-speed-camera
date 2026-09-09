/**
 * @file ili9341.c
 *
 * @brief Implementation of the ILI9341 TFT LCD driver.
 *
 * @details See ili9341.h for the public API documentation. This file
 * 			contains the low-level SPI command/data transaction primitives
 * 			and the panel initialization and addressing logic, implemented
 * 			in accordance with the ILI9341 datasheet's command set and
 * 			power-on sequence.
 */

#include "ili9341.h"

#define ILI9341_CMD_SOFTWARE_RESET		0x01	/**< Software reset command. */
#define ILI9341_CMD_SLEEP_OUT			0x11	/**< Sleep out (exit sleep mode) command. */
#define ILI9341_CMD_DISPLAY_OFF			0x28	/**< Display off command. */
#define ILI9341_CMD_DISPLAY_ON			0x29	/**< Display on command. */
#define ILI9341_CMD_COLUMN_ADDR_SET		0x2A	/**< Column address set (CASET) command. */
#define ILI9341_CMD_PAGE_ADDR_SET		0x2B	/**< Page address set (PASET) command. */
#define ILI9341_CMD_MEMORY_WRITE		0x2C	/**< Memory write (RAMWR) command. */
#define ILI9341_CMD_MEMORY_ACCESS_CTRL	0x36	/**< Memory access control (MADCTL) command - scan direction/orientation. */
#define ILI9341_CMD_PIXEL_FORMAT_SET	0x3A	/**< Pixel format set (COLMOD) command. */

HAL_StatusTypeDef ILI9341_WriteCommand(ILI9341_HandleTypeDef *lcd, uint8_t cmd) {
    if(lcd == NULL) {
        return HAL_ERROR;
    }

    HAL_GPIO_WritePin(lcd->dc_port, lcd->dc_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(lcd->cs_port, lcd->cs_pin, GPIO_PIN_RESET);
    if(HAL_SPI_Transmit(lcd->hspi, &cmd, 1, HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(lcd->cs_port, lcd->cs_pin, GPIO_PIN_SET);
    
    return HAL_OK;
}

HAL_StatusTypeDef ILI9341_WriteData(ILI9341_HandleTypeDef *lcd, uint8_t data) {
    if(lcd == NULL) {
        return HAL_ERROR;
    }

    HAL_GPIO_WritePin(lcd->dc_port, lcd->dc_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(lcd->cs_port, lcd->cs_pin, GPIO_PIN_RESET);
    if(HAL_SPI_Transmit(lcd->hspi, &data, 1, HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(lcd->cs_port, lcd->cs_pin, GPIO_PIN_SET);

    return HAL_OK;
}

HAL_StatusTypeDef ILI9341_SendData(ILI9341_HandleTypeDef *lcd, uint16_t data) {
    if(lcd == NULL) {
        return HAL_ERROR;
    }

    HAL_GPIO_WritePin(lcd->dc_port, lcd->dc_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(lcd->cs_port, lcd->cs_pin, GPIO_PIN_RESET);
    uint8_t rx[2] = {data >> 8, data & 0xFF};
    if(HAL_SPI_Transmit(lcd->hspi, rx, 2, HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(lcd->cs_port, lcd->cs_pin, GPIO_PIN_SET);

    return HAL_OK;
}

lcdStatus_t ILI9341_Init(ILI9341_HandleTypeDef *lcd) {
    if(lcd == NULL) {
        return LCD_ERROR;
    }

    HAL_GPIO_WritePin(lcd->rst_port, lcd->rst_pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(lcd->rst_port, lcd->rst_pin, GPIO_PIN_SET);
    HAL_Delay(100);

    if(ILI9341_WriteCommand(lcd, ILI9341_CMD_SOFTWARE_RESET) != HAL_OK) {
        return LCD_ERROR_WRITE_CMD;
    }
    HAL_Delay(150);

    if(ILI9341_WriteCommand(lcd, ILI9341_CMD_PIXEL_FORMAT_SET) != HAL_OK) {
        return LCD_ERROR_WRITE_CMD;
    }
    if(ILI9341_WriteData(lcd, 0x55) != HAL_OK) {
        return LCD_ERROR_WRITE_DATA;
    }
    HAL_Delay(10);

    if(ILI9341_WriteCommand(lcd, ILI9341_CMD_MEMORY_ACCESS_CTRL) != HAL_OK) {
        return LCD_ERROR_WRITE_CMD;
    }
    if(ILI9341_WriteData(lcd, 0x28) != HAL_OK) {
        return LCD_ERROR_WRITE_DATA;
    }
    HAL_Delay(10);

    if(ILI9341_WriteCommand(lcd, ILI9341_CMD_SLEEP_OUT) != HAL_OK) {
        return LCD_ERROR_WRITE_CMD;
    }
    HAL_Delay(120);

    if(ILI9341_WriteCommand(lcd, ILI9341_CMD_DISPLAY_ON) != HAL_OK) {
        return LCD_ERROR_WRITE_CMD;
    }

    return LCD_OK;
}

lcdStatus_t ILI9341_SetWindow(ILI9341_HandleTypeDef *lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    if(lcd == NULL || x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0) {
        return LCD_ERROR;
    }

    if(ILI9341_WriteCommand(lcd, ILI9341_CMD_COLUMN_ADDR_SET) != HAL_OK) {
        return LCD_ERROR_WRITE_CMD;
    }
    if(ILI9341_WriteData(lcd, x1 >> 8) != HAL_OK) {
        return LCD_ERROR_WRITE_DATA;
    }
    if(ILI9341_WriteData(lcd, x1 & 0xFF) != HAL_OK) {
        return LCD_ERROR_WRITE_DATA;
    }
    if(ILI9341_WriteData(lcd, x2 >> 8) != HAL_OK) {
        return LCD_ERROR_WRITE_DATA;
    }
    if(ILI9341_WriteData(lcd, x2 & 0xFF) != HAL_OK) {
        return LCD_ERROR_WRITE_DATA;
    }

    if(ILI9341_WriteCommand(lcd, ILI9341_CMD_PAGE_ADDR_SET) != HAL_OK) {
        return LCD_ERROR_WRITE_CMD;
    }
    if(ILI9341_WriteData(lcd, y1 >> 8) != HAL_OK) {
        return LCD_ERROR_WRITE_DATA;
    }
    if(ILI9341_WriteData(lcd, y1 & 0xFF) != HAL_OK) {
        return LCD_ERROR_WRITE_DATA;
    }
    if(ILI9341_WriteData(lcd, y2 >> 8) != HAL_OK) {
        return LCD_ERROR_WRITE_DATA;
    }
    if(ILI9341_WriteData(lcd, y2 & 0xFF) != HAL_OK) {
        return LCD_ERROR_WRITE_DATA;
    }

    if(ILI9341_WriteCommand(lcd, ILI9341_CMD_MEMORY_WRITE) != HAL_OK) {
        return LCD_ERROR_WRITE_CMD;
    }

    return LCD_OK;
}

lcdStatus_t ILI9341_Test(ILI9341_HandleTypeDef *lcd, uint16_t color) {
    if(lcd == NULL) {
        return LCD_ERROR;
    }

    if(ILI9341_SetWindow(lcd, 0, 0, 319, 239) != LCD_OK) {
        return LCD_ERROR;
    }
    HAL_GPIO_WritePin(lcd->dc_port, lcd->dc_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(lcd->cs_port, lcd->cs_pin, GPIO_PIN_RESET);

    for(uint32_t i = 0; i < 320*240; i++) {
        uint8_t data[2] = {color >> 8, color & 0xFF};
        if(HAL_SPI_Transmit(lcd->hspi, data, 2, 10) != HAL_OK) {
            return LCD_ERROR;
        }
    }
    HAL_GPIO_WritePin(lcd->cs_port, lcd->cs_pin, GPIO_PIN_SET);

    return LCD_OK;
}
