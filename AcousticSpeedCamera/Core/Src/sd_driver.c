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

static volatile DSTATUS s_stat = STA_NOINIT; /**< Current disk status, as tracked by FatFs (e.g. STA_NOINIT).*/

/**
 * @brief Card handle for the single SD card volume managed by this driver.
 *
 * @details Binds the SD SPI driver do SPI4 and the SD_CS_GPIO pin; passed
 * 			to every SD_Init()/SD_ReadBlock_DMA()/SD_WriteBlock_DMA() call
 * 			made from this file. card_type is populated by SD_disk_init()
 * 			on the first successful initialization.
 */
sdCard_HandleTypeDef sd = {
	.hspi = &hspi4,
	.cs_port = SD_CS_GPIO_Port,
	.cs_pin = SD_CS_Pin,
	.card_type = SD_TYPE_UNKNOWN,
};

DSTATUS SD_disk_init(BYTE pdrv) {
	(void)pdrv;
	sdStatus_t st = SD_Init(&sd);

	if(st == SD_OK) {
		if(HAL_SPI_DeInit(sd.hspi) == HAL_OK) {
			sd.hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;

			if(HAL_SPI_Init(sd.hspi) == HAL_OK) {
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
		if(SD_ReadBlock_DMA(&sd, sector + i, buf + i * 512) != SD_OK) {
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
		if(SD_WriteBlock_DMA(&sd, sector + i, buf + i * 512) != SD_OK) {
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
