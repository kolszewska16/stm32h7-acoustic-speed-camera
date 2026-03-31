#include "os_objects.h"

osThreadId_t audioTaskHandle = NULL;
const osThreadAttr_t audioTask_attr = {
	.name = "audioTask",
	.stack_size = 10 * 1024,
	.priority = (osPriority_t) osPriorityHigh,
};
