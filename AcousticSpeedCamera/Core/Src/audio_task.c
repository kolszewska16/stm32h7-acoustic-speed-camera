#include "audio_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "os_objects.h"
#include "audio_processor.h"
#include "logger.h"
#include "ui.h"

void vAudioTask(void *parameter) {
	LOG_INFO("audio task start");

	while(!ui_ready) {
		osDelay(1);
	}

	static float32_t dBA_max = -1000.0f;
	audio_dsp_init();
	if(audio_hardware_start() != HAL_OK) {
		LOG_ERROR("DFSDM: initialization failed");
	}

	LOG_INFO("initialization completed");

	while(1) {
		uint32_t flags = osThreadFlagsWait(0x03, osFlagsWaitAny, osWaitForever);
		uint32_t part_idx = -1;
		if(flags & 0x01) {
			part_idx = 0;
		}
		else if(flags & 0x02) {
			part_idx = 1;
		}

		uint32_t offset = part_idx * SAMPLES;
		float32_t dBA_avg = process_audio_frame(dmabuff_L, dmabuff_R, offset);

		if(dBA_avg > dBA_max) {
			dBA_max = dBA_avg;
		}

		update_ui(dBA_avg, dBA_max);
	}

	vTaskDelete(NULL);
}
