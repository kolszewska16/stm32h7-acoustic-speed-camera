/**
 * @file sd_driver.c
 *
 * @brief FatFs disk I/O glue layer implementation for the SD SPI driver.
 *
 * @details Implements the disk functions declared in sd_driver.h, bridging
 * 			the hardware-agnostic FatFs library to the SD SPI driver
 * 			(sd_spi.h/.c). See sd_driver.h for the documentation of each
 * 			function's parameters, return values and behavior.
 */

#include "sd_driver.h"
#include "sd_spi.h"

static volatile DSTATUS s_stat = STA_NOINIT; /**< Current disk status, as tracked by FatFs (e.g. STA_NOINIT).*/

DSTATUS SD_disk_init(BYTE pdrv) {
	(void)pdrv;
	sdStatus_t st = SD_Init(&hspi4, SD_CS_GPIO_Port, SD_CS_Pin);

	if(st == SD_OK) {
		if(HAL_SPI_DeInit(&hspi4) == HAL_OK) {
			hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;

			if(HAL_SPI_Init(&hspi4) == HAL_OK) {
				s_stat &= ~STA_NOINIT;
			}
			else {
				s_stat = STA_NOINIT;
			}
		}
		else {
			s_stat = STA_NOINIT;
		}
	}
	else {
		s_stat = STA_NOINIT;
	}

	return s_stat;
}

DSTATUS SD_disk_status(BYTE pdrv) {
	(void)pdrv;
	return s_stat;
}

DRESULT SD_disk_read(BYTE pdrv, BYTE *buf, DWORD sector, UINT count) {
	(void)pdrv;
	if(s_stat & STA_NOINIT) {
		return RES_NOTRDY;
	}

	for(UINT i = 0; i < count; i++) {
		if(SD_ReadBlock_DMA(sector + i, buf + i * 512) != SD_OK) {
			return RES_ERROR;
		}
	}

	return RES_OK;
}

DRESULT SD_disk_write(BYTE pdrv, const BYTE *buf, DWORD sector, UINT count) {
	(void)pdrv;
	if(s_stat & STA_NOINIT) {
		return RES_NOTRDY;
	}

	for(UINT i = 0; i < count; i++) {
		if(SD_WriteBlock_DMA(sector + i, buf + i * 512) != SD_OK) {
			return RES_ERROR;
		}
	}

	return RES_OK;
}

DRESULT SD_disk_ioctl(BYTE pdrv, BYTE cmd, void *buf) {
	(void)pdrv;
	switch(cmd) {
		case CTRL_SYNC:
			return RES_OK;

		case GET_SECTOR_SIZE:
			*(WORD *)buf = 512;
			return RES_OK;

		case GET_BLOCK_SIZE:
			*(DWORD *)buf = 1;
			return RES_OK;

		case GET_SECTOR_COUNT:
			// TODO
			// parse CSD (CMD9)

			*(DWORD *)buf = 8000000UL;
			return RES_OK;

		default:
			return RES_PARERR;
	}
}

Diskio_drvTypeDef SD_Driver = {
	SD_disk_init,
	SD_disk_status,
	SD_disk_read,
	SD_disk_write,
	SD_disk_ioctl,
};
