#ifndef INC_SD_UTILS_H_
#define INC_SD_UTILS_H_

#include <stdint.h>
#include "ff.h"

#define SD_SPACE_WARN_THRESHOLD_BYTES (10ULL * 1024 * 1024)	// 10 MB

typedef enum {
	SD_SPACE_OK,
	SD_SPACE_LOW,
	SD_SPACE_FULL,
} SD_SpaceStatus_t;

FRESULT SD_GetFreeSpace(const TCHAR *path, uint64_t *free_bytes,
	uint64_t *total_bytes);

SD_SpaceStatus_t SD_CheckSpaceBeforeWrite(const TCHAR *path,
	uint64_t size_needed, uint64_t *free_bytes);

#endif /* INC_SD_UTILS_H_ */
