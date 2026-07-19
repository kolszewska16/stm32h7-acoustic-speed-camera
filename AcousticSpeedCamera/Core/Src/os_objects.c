#include "os_objects.h"

osSemaphoreId_t s_spi_dma_sem;

QueueHandle_t xLogQueue;

osMutexId_t uartMutex;

osMutexAttr_t uartMutex_attr = {
	.name = "uartMutex",
	.attr_bits = osMutexRecursive | osMutexPrioInherit,
	.cb_mem = NULL,
	.cb_size = 0U,
};

osThreadId_t audioTaskHandle = NULL;
osThreadId_t sdLoggerTaskHandle = NULL;

const osThreadAttr_t audioTask_attr = {
	.name = "audioTask",
	.stack_size = 10 * 1024,
	.priority = (osPriority_t) osPriorityHigh,
};

const osThreadAttr_t sdLoggerTask_attr = {
	.name = "sdLoggerTask",
	.stack_size = 4 * 1024,
	.priority = (osPriority_t) osPriorityNormal,
};
