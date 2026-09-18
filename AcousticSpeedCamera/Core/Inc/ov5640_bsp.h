#ifndef INC_OV5640_BSP_H_
#define INC_OV5640_BSP_H_

#include <stdint.h>
#include "main.h"
#include "ov5640.h"

#define OV5640_I2C_ADDRESS 0x78
#define OV5640_I2C_TIMEOUT_MS 100

typedef struct {
	I2C_HandleTypeDef *hi2c;

	GPIO_TypeDef *rst_port;
	uint16_t rst_pin;

	GPIO_TypeDef *pwdn_port;
	uint16_t pwdn_pin;
} OV5640_HandleTypedef;

int32_t OV5640_BSP_Init(OV5640_Object_t *pObj, OV5640_HandleTypedef *cam);

int32_t OV5640_BSP_DeInit(OV5640_Object_t *pObj, OV5640_HandleTypedef *cam);

#endif /* INC_OV5640_BSP_H_ */
