#ifndef INC_AUDIO_TASK_H_
#define INC_AUDIO_TASK_H_

#include "arm_math.h"

typedef struct {
	float32_t dBA_avg;
	float32_t dBA_max;
} AudioLevelUpdate_t;

void vAudioTask(void *parameter);

#endif /* INC_AUDIO_TASK_H_ */
