#ifndef INC_SD_LOGGER_H_
#define INC_SD_LOGGER_H_

#include <stdint.h>
#include "arm_math.h"

typedef struct {
	uint8_t *data;
	uint32_t len;
	uint32_t timestamp_ms;
} ImageSaveRequest_t;

void vSDLogTask(void *argument);

#endif /* INC_SD_LOGGER_H_ */
