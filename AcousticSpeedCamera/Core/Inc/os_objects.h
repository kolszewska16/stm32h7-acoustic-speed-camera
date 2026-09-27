#ifndef INC_OS_OBJECTS_H_
#define INC_OS_OBJECTS_H_

#include <stdbool.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "semphr.h"

#define UI_STATUS_MSG_MAX_LEN 64

typedef struct {
	char text[UI_STATUS_MSG_MAX_LEN];
} UiStatusMsg_t;

extern osSemaphoreId_t s_spi_dma_sem;
extern QueueHandle_t xImageQueue;
extern QueueHandle_t xUiAudioStatusQueue;
extern QueueHandle_t xUiSdStatusQueue;
extern QueueHandle_t xUiBatteryStatusQueue;
extern QueueHandle_t xUiStatusQueue;

extern volatile bool camera_ready;
extern volatile bool sd_ready;
extern volatile bool ui_ready;

extern osMutexId_t uartMutex;
extern osMutexAttr_t uartMutex_attr;

extern osThreadId_t audioTaskHandle;
extern osThreadId_t sdLoggerTaskHandle;
extern osThreadId_t displayTaskHandle;
extern osThreadId_t cameraTaskHandle;
extern osThreadId_t batteryTaskHandle;

extern const osThreadAttr_t audioTask_attr;
extern const osThreadAttr_t sdLoggerTask_attr;
extern const osThreadAttr_t displayTask_attr;
extern const osThreadAttr_t cameraTask_attr;
extern const osThreadAttr_t batteryTask_attr;

#endif /* INC_OS_OBJECTS_H_ */
