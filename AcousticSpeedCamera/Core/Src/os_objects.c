#include "os_objects.h"

osMutexId_t uartMutex;

osMutexAttr_t uartMutex_attr = {
	.name = "uartMutex",
	.attr_bits = osMutexRecursive | osMutexPrioInherit,
	.cb_mem = NULL,
	.cb_size = 0U,
};

osThreadId_t audioTaskHandle = NULL;
osThreadId_t cameraTaskHandle = NULL;

const osThreadAttr_t audioTask_attr = {
	.name = "audioTask",
	.stack_size = 10 * 1024,
	.priority = (osPriority_t) osPriorityHigh,
};

const osThreadAttr_t cameraTask_attr = {
	.name = "cameraTask",
	.stack_size = 4 * 1024,
	.priority = (osPriority_t) osPriorityNormal,
};
