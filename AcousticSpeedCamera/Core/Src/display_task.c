#include "display_task.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "os_objects.h"

#define HORIZONTAL_RESOLUTION 320
#define VERTICAL_RESOLUTION 240
#define BYTES_PER_PIXEL LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)

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

void flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
	if(display == NULL || area == NULL || px_map == NULL) {
		return;
	}

	// TODO
	// optimize flush_cb function by using DMA

	ILI9341_SetWindow(&lcd, area->x1, area->y1, area->x2, area->y2);
	uint16_t *buf16 = (uint16_t*)px_map;
	int32_t x, y;
	for(y = area->y1; y <= area->y2; y++) {
		for(x = area->x1; x <= area->x2; x++) {
			ILI9341_SendData(&lcd, *buf16);
			buf16++;
		}
	}

	lv_display_flush_ready(display);
}

void vDisplayTask(void *parameter) {
	if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
		const char *msg = "[INFO] display task start\r\n";
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		osMutexRelease(uartMutex);
	}

	lv_init();
	HAL_GPIO_WritePin(lcd.bl_port, lcd.bl_pin, GPIO_PIN_SET);
	ILI9341_Init(&lcd);
	lv_tick_set_cb(HAL_GetTick);
	lv_display_t *display1 = lv_display_create(HORIZONTAL_RESOLUTION, VERTICAL_RESOLUTION);
	static uint8_t buf1[HORIZONTAL_RESOLUTION * VERTICAL_RESOLUTION / 10 * BYTES_PER_PIXEL];
	lv_display_set_buffers(display1, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_flush_cb(display1, flush_cb);

	lv_obj_t *label = lv_label_create(lv_screen_active());
	lv_label_set_text(label, "HELLO WORLD!");
	lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

	while(1) {
		lv_timer_handler();
		osDelay(pdMS_TO_TICKS(5));
/*		ILI9341_Test(&lcd, 0xF800);		// red
		osDelay(pdMS_TO_TICKS(1000));
		ILI9341_Test(&lcd, 0x07E0);		// green
		osDelay(pdMS_TO_TICKS(1000));
		ILI9341_Test(&lcd, 0x001F);		// blue
		osDelay(pdMS_TO_TICKS(1000));*/
	}

	vTaskDelete(NULL);
}
