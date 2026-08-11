#include "camera_task.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "os_objects.h"
#include "arducam_ov2640.h"

ArduCam_HandleTypedef cam = {
	.hi2c = &hi2c1,
	.hspi = &hspi5,
	.cs_port = CAM_CS_GPIO_Port,
	.cs_pin = CAM_CS_Pin,
};

void vCameraTask(void *parameter) {
	osDelay(250);
	if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
		const char *msg = "[INFO] camera task start\r\n";
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		osMutexRelease(uartMutex);
	}

	uint8_t cam_addr = 0x00;
	if(ArduCam_I2C_ready(&cam, &cam_addr) == CAMERA_OK) {
		if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
			char msg[64];
			snprintf(msg, sizeof(msg), "[INFO] I2C camera address: 0x%02X\r\n", cam_addr);
			HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			osMutexRelease(uartMutex);
		}
	}

	uint8_t pidh = 0x00;
	uint8_t pidl = 0x00;
	if(ArduCam_I2C_read_chip(&cam, &pidh, &pidl) == CAMERA_OK) {
		if(osMutexAcquire(uartMutex, osWaitForever) == osOK) {
			char msg[64];
			snprintf(msg, sizeof(msg), "[INFO] chip id: pidh=0x%02X pidl=0x%02X\r\n", pidh, pidl);
			HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			osMutexRelease(uartMutex);
		}
	}

	while(1) {
		cameraStatus_t status = ArduCam_Init(&cam);
		if(status != CAMERA_OK) {
			HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
		}
		osDelay(pdMS_TO_TICKS(500));
		HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
	}

	vTaskDelete(NULL);
}
