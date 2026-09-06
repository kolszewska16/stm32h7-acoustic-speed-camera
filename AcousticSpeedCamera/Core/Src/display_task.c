#include "display_task.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "os_objects.h"
#include "ui.h"

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

	display_init(&lcd);
	lv_lock();
	measurement_screen_init();
	lv_refr_now(NULL);
	ui_ready = true;
	lv_unlock();

	while(1) {
		lv_timer_handler();
		osDelay(pdMS_TO_TICKS(5));
	}

	vTaskDelete(NULL);
}
