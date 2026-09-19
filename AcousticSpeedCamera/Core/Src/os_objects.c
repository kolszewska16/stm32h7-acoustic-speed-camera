#include "os_objects.h"

volatile bool camera_ready = false;
volatile bool sd_ready = false;
volatile bool ui_ready = false;

osMutexId_t uartMutex = NULL;

osMutexAttr_t uartMutex_attr = {
	.name = "uartMutex",
	.attr_bits = osMutexRecursive | osMutexPrioInherit,
	.cb_mem = NULL,
	.cb_size = 0U,
};

osThreadId_t audioTaskHandle = NULL;
osThreadId_t displayTaskHandle = NULL;
osThreadId_t cameraTaskHandle = NULL;

const osThreadAttr_t audioTask_attr = {
	.name = "audioTask",
	.stack_size = 10 * 1024,
	.priority = (osPriority_t) osPriorityAboveNormal,
};

const osThreadAttr_t displayTask_attr = {
	.name = "displayTask",
	.stack_size = 5 * 1024,
	.priority = (osPriority_t) osPriorityBelowNormal,
};

const osThreadAttr_t cameraTask_attr = {
	.name = "cameraTask",
	.stack_size = 10 * 1024,
	.priority = (osPriority_t) osPriorityAboveNormal,
};
