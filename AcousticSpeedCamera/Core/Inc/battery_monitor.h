#ifndef INC_BATTERY_MONITOR_H_
#define INC_BATTERY_MONITOR_H_

#include <stdint.h>
#include "main.h"

typedef struct {
	float v;
	uint8_t pct;
} SocPoint_t;

extern ADC_HandleTypeDef hadc3;

float Battery_ReadVoltage(void);
uint8_t Battery_VoltageToPercent(float v_pack);

void vBatteryTask(void *parameter);

#endif /* INC_BATTERY_MONITOR_H_ */
