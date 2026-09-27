#include "display_task.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "os_objects.h"
#include "ui.h"
#include "logger.h"
#include "audio_task.h"

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
	LOG_INFO("display task start");
	display_init(&lcd);

	lv_lock();
	measurement_screen_init();
	lv_refr_now(NULL);
	ui_ready = true;
	lv_unlock();

	AudioLevelUpdate_t levels;
	uint8_t sd_percent;
	uint8_t battery_percent;
	UiStatusMsg_t status_msg;

	while(1) {
		if(xQueueReceive(xUiAudioStatusQueue, &levels, 0) == pdTRUE) {
			update_audio_ui(levels.dBA_avg, levels.dBA_max);
		}

		if(xQueueReceive(xUiSdStatusQueue, &sd_percent, 0) == pdTRUE) {
			update_sd_status_label(sd_percent);
		}

		if(xQueueReceive(xUiBatteryStatusQueue, &battery_percent, 0) == pdTRUE) {
			update_battery_status_label(battery_percent);
		}

		if(xQueueReceive(xUiStatusQueue, &status_msg, 0) == pdTRUE) {
			update_status_bar(status_msg.text);
		}

		lv_lock();
		lv_timer_handler();
		lv_unlock();
		osDelay(pdMS_TO_TICKS(5));
	}

	vTaskDelete(NULL);
}
