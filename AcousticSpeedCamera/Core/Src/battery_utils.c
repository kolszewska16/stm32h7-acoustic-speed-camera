#include "battery_utils.h"

static const SocPoint_t soc_table[] = {
	{4.20f, 100}, {4.11f, 90}, {4.02f, 80}, {3.95f, 70}, {3.87f, 60},
	{3.84f, 50},  {3.80f, 40}, {3.77f, 30}, {3.73f, 20}, {3.69f, 10},
	{3.61f, 5},   {3.27f, 0},
};
#define SOC_TABLE_LEN (sizeof(soc_table) / sizeof(soc_table[0]))

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
