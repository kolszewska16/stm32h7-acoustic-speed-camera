#ifndef INC_ILI9341_H_
#define INC_ILI9341_H_

#include <stdint.h>
#include "main.h"

#define ILI9341_CMD_SOFTWARE_RESET 0x01
#define ILI9341_CMD_SLEEP_OUT 0x11
#define ILI9341_CMD_DISPLAY_OFF 0x28
#define ILI9341_CMD_DISPLAY_ON 0x29
#define ILI9341_CMD_COLUMN_ADDR_SET 0x2A
#define ILI9341_CMD_PAGE_ADDR_SET 0x2B
#define ILI9341_CMD_MEMORY_WRITE 0x2C
#define ILI9341_CMD_MEMORY_ACCESS_CTRL 0x36
#define ILI9341_CMD_PIXEL_FORMAT_SET 0x3A

typedef struct {
    SPI_HandleTypeDef *hspi;

    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;

    GPIO_TypeDef *dc_port;
    uint16_t dc_pin;

    GPIO_TypeDef *rst_port;
    uint16_t rst_pin;

    GPIO_TypeDef *bl_port;
    uint16_t bl_pin;
} ILI9341_HandleTypeDef;

typedef enum {
    LCD_OK,
    LCD_ERROR,
    LCD_ERROR_WRITE_CMD,
    LCD_ERROR_WRITE_DATA,
    LCD_ERROR_SEND_DATA,
} lcdStatus_t;

HAL_StatusTypeDef ILI9341_WriteCommand(ILI9341_HandleTypeDef *lcd, uint8_t cmd);
HAL_StatusTypeDef ILI9341_WriteData(ILI9341_HandleTypeDef *lcd, uint8_t data);
HAL_StatusTypeDef ILI9341_SendData(ILI9341_HandleTypeDef *lcd, uint16_t data);

lcdStatus_t ILI9341_Init(ILI9341_HandleTypeDef *lcd);
lcdStatus_t ILI9341_SetWindow(ILI9341_HandleTypeDef *lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
lcdStatus_t ILI9341_Test(ILI9341_HandleTypeDef *lcd, uint16_t color);

#endif /* INC_ILI9341_H_ */
