#include <i2c_diagnostics.h>
#include "camera_task.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "os_objects.h"

void vCameraTask(void *parameter) {
	osDelay(250);
	if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
		const char *msg = "[INFO] camera task start\r\n";
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		osMutexRelease(uartMutex);
	}

	int ret = i2c_is_ready();
	if(ret != 0) {
		i2c_scan_bus();
	}
	else {
		i2c_read_chip();
	}

	while(1) {
		//
	}

	vTaskDelete(NULL);
}
