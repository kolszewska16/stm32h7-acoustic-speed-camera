#ifndef INC_SD_LOGGER_H_
#define INC_SD_LOGGER_H_

#include <stdint.h>
#include "arm_math.h"

typedef struct {
	uint32_t timestamp_ms;
	float32_t power;
	float32_t dbspl_avg;
} LogEntry_t;

void vSDLogTask(void *argument);

#endif /* INC_SD_LOGGER_H_ */
