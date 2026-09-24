#include "battery_monitor.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"
#include "os_objects.h"
#include "ui.h"
#include "logger.h"

#define ADC_VREF 3.3f
#define ADC_FULL_SCALE 4095.0f
#define R_TOP 51000.0f
#define R_BOT 16000.0f
#define DIV_RATIO ((R_TOP + R_BOT) / R_BOT)
#define N_SAMPLES 10
#define BAT_CELLS 3
#define EMA_ALPHA 0.2f

static const SocPoint_t soc_table[] = {
	{4.20f, 100}, {4.11f, 90}, {4.02f, 80}, {3.95f, 70}, {3.87f, 60},
	{3.84f, 50},  {3.80f, 40}, {3.77f, 30}, {3.73f, 20}, {3.69f, 10},
	{3.61f, 5},   {3.27f, 0},
};
#define SOC_TABLE_LEN (sizeof(soc_table) / sizeof(soc_table[0]))

static volatile float s_bat_voltage = 0.0f;
static volatile uint8_t s_bat_percent = 0;

float Battery_ReadVoltage(void) {
	uint32_t sum = 0;

	for(int i = 0; i < N_SAMPLES; i++) {
		HAL_ADC_Start(&hadc3);
		if(HAL_ADC_PollForConversion(&hadc3, 10) == HAL_OK) {
			sum += HAL_ADC_GetValue(&hadc3);
		}
	}

	float adc_avg = (float)sum / N_SAMPLES;
	float v_pin = adc_avg * ADC_VREF / ADC_FULL_SCALE;

	return v_pin * DIV_RATIO;
}

uint8_t Battery_VoltageToPercent(float v_pack) {
	float v = v_pack / BAT_CELLS;

	if(v >= soc_table[0].v) {
		return 100;
	}
	if(v <= soc_table[SOC_TABLE_LEN - 1].v) {
		return 0;
	}

	for(uint32_t i = 0; i < SOC_TABLE_LEN - 1; i++) {
		if(v <= soc_table[i].v && v > soc_table[i + 1].v) {
			float span = soc_table[i].v - soc_table[i + 1].v;
			float frac = (v - soc_table[i + 1].v) / span;
			return (uint8_t)(soc_table[i + 1].pct +
				frac * (soc_table[i].pct - soc_table[i + 1].pct));
		}
	}

	return 0;
}

void vBatteryTask(void *parameter) {
	LOG_INFO("battery task start");

	while(!ui_ready) {
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

		update_battery_status_label(s_bat_percent);

		osDelay(pdMS_TO_TICKS(1000));
	}
}
