#include <i2c_diagnostics.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "os_objects.h"

#define OV2640_I2C_ADDR 0x30 // 0x60 >> 1 = 0x30

int i2c_is_ready(void) {
	if(HAL_I2C_IsDeviceReady(&hi2c1, OV2640_I2C_ADDR << 1, 10, 100) == HAL_OK) {
		if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
			char msg[128];
			snprintf(msg, sizeof(msg), "[INFO] I2C device found at 0x%02X\r\n", OV2640_I2C_ADDR);
			HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			osMutexRelease(uartMutex);
		}
		return 0;
	}
	else {
		if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
			char msg[128];
			snprintf(msg, sizeof(msg), "[ERROR] I2C device not found at 0x%02X\r\n", OV2640_I2C_ADDR);
			HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			osMutexRelease(uartMutex);
		}
		return -1;
	}
}

void i2c_scan_bus(void) {
	if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
		const char *msg = "[INFO] scanning I2C bus...\r\n";
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		osMutexRelease(uartMutex);
	}
	int dev_count = 0;
	for(uint8_t addr = 1; addr < 128; addr++) {
		if(HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 100) == HAL_OK) {
			if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
				char msg[128];
				snprintf(msg, sizeof(msg), "[INFO] I2C device found at 0x%02X\r\n", addr);
				HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
				osMutexRelease(uartMutex);
			}
			dev_count++;
		}
	}
	if(dev_count == 0) {
		if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
			const char *msg = "[INFO] no devices found at I2C bus\r\n";
			HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			osMutexRelease(uartMutex);
		}
	}
	else {
		if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
			char msg[128];
			snprintf(msg, sizeof(msg), "[INFO] found %d devices at I2C bus\r\n", dev_count);
			HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			osMutexRelease(uartMutex);
		}
	}
}

void i2c_read_chip(void) {
	uint8_t reg;
	uint8_t pidh;
	uint8_t pidl;

	// register bank select
	reg = 0xFF;
	uint8_t bank_select = 0x01;
	uint8_t buf[2] = {reg, bank_select};
	if(HAL_I2C_Master_Transmit(&hi2c1, OV2640_I2C_ADDR << 1, buf, sizeof(buf), HAL_MAX_DELAY) != HAL_OK) {
		if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
			const char *msg = "[ERROR] failed to save bank select\r\n";
			HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			osMutexRelease(uartMutex);
		}
		return;
	}

	// PIDH reading (0x0A)
	reg = 0x0A;
	if(HAL_I2C_Master_Transmit(&hi2c1, OV2640_I2C_ADDR << 1, &reg, 1, HAL_MAX_DELAY) == HAL_OK) {
		if(HAL_I2C_Master_Receive(&hi2c1, OV2640_I2C_ADDR << 1, &pidh, 1, HAL_MAX_DELAY) != HAL_OK) {
			if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
				const char *msg = "[ERROR] failed to read PIDH\r\n";
				HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
				osMutexRelease(uartMutex);
			}
			return;
		}
	}

	// PIDL reading (0x0B)
	reg = 0x0B;
	if(HAL_I2C_Master_Transmit(&hi2c1, OV2640_I2C_ADDR << 1, &reg, 1, HAL_MAX_DELAY) == HAL_OK) {
		if(HAL_I2C_Master_Receive(&hi2c1, OV2640_I2C_ADDR << 1, &pidl, 1, HAL_MAX_DELAY) != HAL_OK) {
			if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
				const char *msg = "[ERROR] failed to read PIDL\r\n";
				HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
				osMutexRelease(uartMutex);
			}
			return;
		}
	}

	if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
		char msg[128];
		snprintf(msg, sizeof(msg), "[INFO] I2C device chip ID: %02X %02X\r\n", pidh, pidl);
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		osMutexRelease(uartMutex);
	}
}
