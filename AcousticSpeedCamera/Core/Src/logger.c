#include "logger.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "FreeRTOS.h"
#include "main.h"
#include "os_objects.h"

#define LOG_BUF_SIZE 128

void log_msg(log_level_t level, const char *msg, ...) {
	if(msg == NULL) {
		return;
	}

	char buf[LOG_BUF_SIZE];
	int offset = 0;

	switch(level) {
		case LOG_LEVEL_INFO:
			offset = snprintf(buf, sizeof(buf), "[INFO] ");
			break;

		case LOG_LEVEL_WARNING:
			offset = snprintf(buf, sizeof(buf), "[WARNING] ");
			break;

		case LOG_LEVEL_ERROR:
			offset = snprintf(buf, sizeof(buf), "[ERROR] ");
			break;
	}

	if(offset < sizeof(buf) - 3) {
		va_list args;
		va_start(args, msg);
		vsnprintf(buf + offset, sizeof(buf) - offset - 3, msg, args);
		va_end(args);
	}

	size_t len = strlen(buf);

	buf[len++] = '\r';
	buf[len++] = '\n';
	buf[len] = '\0';

	if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)buf, (uint16_t)len, 10);
		osMutexRelease(uartMutex);
	}
}
