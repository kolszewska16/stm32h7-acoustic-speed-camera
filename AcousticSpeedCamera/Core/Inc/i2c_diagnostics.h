#ifndef INC_I2C_DIAGNOSTICS_H_
#define INC_I2C_DIAGNOSTICS_H_

#include "main.h"

extern I2C_HandleTypeDef hi2c1;

int i2c_is_ready(void);
void i2c_scan_bus(void);
void i2c_read_chip(void);

#endif /* INC_I2C_DIAGNOSTICS_H_ */
