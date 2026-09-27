#include "sd_utils.h"

#define SD_SECTOR_SIZE 512U

FRESULT SD_GetFreeSpace(const TCHAR *path, uint64_t *free_bytes,
	uint64_t *total_bytes)
{
	FATFS *fs;
	DWORD free_clusters;

	FRESULT fr = f_getfree(path, &free_clusters, &fs);
	if(fr != FR_OK) {
		return fr;
	}

	uint64_t bytes_per_cluster = (uint64_t)fs->csize * SD_SECTOR_SIZE;
	*free_bytes = (uint64_t)free_clusters * bytes_per_cluster;
	*total_bytes = (uint64_t)(fs->n_fatent - 2) * bytes_per_cluster;

	return FR_OK;
}

SD_SpaceStatus_t SD_CheckSpaceBeforeWrite(const TCHAR *path, uint64_t size_needed,
	uint64_t *free_bytes)
{
	uint64_t free_b;
	uint64_t total_b;

	if(SD_GetFreeSpace(path, &free_b, &total_b) != FR_OK) {
		return SD_SPACE_FULL;
	}

	if(free_bytes) {
		*free_bytes = free_b;
	}

	if(free_b < size_needed) {
		return SD_SPACE_FULL;
	}

	if((free_b - size_needed) < SD_SPACE_WARN_THRESHOLD_BYTES) {
		return SD_SPACE_LOW;
	}

	return SD_SPACE_OK;
}
