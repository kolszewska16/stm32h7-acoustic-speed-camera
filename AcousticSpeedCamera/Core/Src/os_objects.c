#include "os_objects.h"

osSemaphoreId_t s_spi_dma_sem;
QueueHandle_t xLogQueue;

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
osThreadId_t sdLoggerTaskHandle = NULL;
osThreadId_t displayTaskHandle = NULL;
osThreadId_t cameraTaskHandle = NULL;
osThreadId_t batteryTaskHandle = NULL;

const osThreadAttr_t audioTask_attr = {
	.name = "audioTask",
	.stack_size = 10 * 1024,
	.priority = (osPriority_t) osPriorityAboveNormal,
};

const osThreadAttr_t displayTask_attr = {
	.name = "displayTask",
	.stack_size = 5 * 1024,
	.priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t cameraTask_attr = {
	.name = "cameraTask",
	.stack_size = 15 * 1024,
	.priority = (osPriority_t) osPriorityAboveNormal,
};

const osThreadAttr_t batteryTask_attr = {
	.name = "batteryTask",
	.stack_size = 3 * 1024,
	.priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t sdLoggerTask_attr = {
	.name = "sdLoggerTask",
	.stack_size = 4 * 1024,
	.priority = (osPriority_t) osPriorityNormal,
};
