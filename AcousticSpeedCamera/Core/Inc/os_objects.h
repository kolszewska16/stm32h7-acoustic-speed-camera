#ifndef INC_OS_OBJECTS_H_
#define INC_OS_OBJECTS_H_

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "semphr.h"

extern osSemaphoreId_t s_spi_dma_sem;

extern QueueHandle_t xLogQueue;

extern osMutexId_t uartMutex;
extern osMutexAttr_t uartMutex_attr;

extern osThreadId_t audioTaskHandle;
extern osThreadId_t sdLoggerTaskHandle;

extern const osThreadAttr_t audioTask_attr;
extern const osThreadAttr_t sdLoggerTask_attr;

#endif /* INC_OS_OBJECTS_H_ */
