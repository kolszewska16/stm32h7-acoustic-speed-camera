#ifndef INC_BATTERY_UTILS_H_
#define INC_BATTERY_UTILS_H_

#include <stdint.h>
#include "main.h"

#define ADC_VREF 3.3f
#define ADC_FULL_SCALE 4095.0f
#define R_TOP 51000.0f
#define R_BOT 16000.0f
#define DIV_RATIO ((R_TOP + R_BOT) / R_BOT)
#define N_SAMPLES 10
#define BAT_CELLS 3
#define EMA_ALPHA 0.2f

typedef struct {
	float v;
	uint8_t pct;
} SocPoint_t;

extern ADC_HandleTypeDef hadc3;

float Battery_ReadVoltage(void);
uint8_t Battery_VoltageToPercent(float v_pack);

#endif /* INC_BATTERY_UTILS_H_ */
