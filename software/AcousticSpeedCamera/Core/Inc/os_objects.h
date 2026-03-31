#ifndef INC_OS_OBJECTS_H_
#define INC_OS_OBJECTS_H_

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "semphr.h"

extern osThreadId_t audioTaskHandle;
extern const osThreadAttr_t audioTask_attr;

#endif /* INC_OS_OBJECTS_H_ */
