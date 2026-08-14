#ifndef INC_OS_OBJECTS_H_
#define INC_OS_OBJECTS_H_

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "semphr.h"

extern osMutexId_t uartMutex;
extern osMutexId_t lvglMutex;

extern osMutexAttr_t uartMutex_attr;
extern osMutexAttr_t lvglMutex_attr;

extern osThreadId_t audioTaskHandle;
extern osThreadId_t displayTaskHandle;

extern const osThreadAttr_t audioTask_attr;
extern const osThreadAttr_t displayTask_attr;

#endif /* INC_OS_OBJECTS_H_ */
