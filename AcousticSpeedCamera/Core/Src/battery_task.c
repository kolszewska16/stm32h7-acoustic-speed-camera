#include "battery_task.h"
#include <string.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"
#include "os_objects.h"
#include "battery_utils.h"
#include "logger.h"

static volatile float s_bat_voltage = 0.0f;
static volatile uint8_t s_bat_percent = 0;
static uint8_t s_last_bat_percent = 0;

static const char* battery_status_msg(uint8_t battery_percent) {
	if(battery_percent <= 10) {
		return "[WARNING] battery critical";
	}
	else if(battery_percent <= 25) {
		return "[INFO] battery low";
	}

	return NULL;
}

static void update_ui_battery_status(uint8_t battery_percent) {
	xQueueOverwrite(xUiBatteryStatusQueue, &battery_percent);
}

static void update_ui_status_bar(const char *msg) {
	UiStatusMsg_t status_msg;
	strncpy(status_msg.text, msg, UI_STATUS_MSG_MAX_LEN - 1);
	status_msg.text[UI_STATUS_MSG_MAX_LEN - 1] = '\0';

	xQueueSend(xUiStatusQueue, &status_msg, 0);
}

static void check_battery_status(uint8_t battery_percent) {
	const char *msg = battery_status_msg(battery_percent);
	if(msg != NULL && (s_last_bat_percent > 25 ||
		(s_last_bat_percent > 10 && battery_percent <= 10)))
	{
		update_ui_status_bar(msg);
	}

	s_last_bat_percent = battery_percent;
}

void vBatteryTask(void *parameter) {
	LOG_INFO("battery task start");

	while(!camera_ready || !ui_ready) {
		osDelay(50);
	}

	if(HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET,
		ADC_SINGLE_ENDED) != HAL_OK)
	{
		LOG_ERROR("ADC: initialization failed");
		vTaskDelete(NULL);
		return;
	}

	float filtered = Battery_ReadVoltage();
	LOG_INFO("battery monitor initialization completed");

	while(1) {
		float v = Battery_ReadVoltage();
		filtered += EMA_ALPHA * (v - filtered);

		s_bat_voltage = filtered;
		s_bat_percent = Battery_VoltageToPercent(filtered);

		update_ui_battery_status(s_bat_percent);
		check_battery_status(s_bat_percent);

		osDelay(pdMS_TO_TICKS(1000));
	}
}
