#include "display_task.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "os_objects.h"
#include "ili9341.h"

ILI9341_HandleTypeDef lcd = {
	.hspi = &hspi3,

	.cs_port = LCD_CS_GPIO_Port,
	.cs_pin = LCD_CS_Pin,

	.dc_port = LCD_DC_GPIO_Port,
	.dc_pin = LCD_DC_Pin,

	.rst_port = LCD_RST_GPIO_Port,
	.rst_pin = LCD_RST_Pin,

	.bl_port = LCD_BL_GPIO_Port,
	.bl_pin = LCD_BL_Pin,
};

void vDisplayTask(void *parameter) {
	if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
		const char *msg = "[INFO] display task start\r\n";
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		osMutexRelease(uartMutex);
	}

	HAL_GPIO_WritePin(lcd.bl_port, lcd.bl_pin, GPIO_PIN_SET);
	ILI9341_Init(&lcd);

	while(1) {
		ILI9341_Test(&lcd, 0xF800);		// red
		osDelay(pdMS_TO_TICKS(1000));
		ILI9341_Test(&lcd, 0x07E0);		// green
		osDelay(pdMS_TO_TICKS(1000));
		ILI9341_Test(&lcd, 0x001F);
		osDelay(pdMS_TO_TICKS(1000));	// blue
	}

	vTaskDelete(NULL);
}
