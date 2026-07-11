#include "ff_gen_drv.h"
#include "sd_spi.h"
#include "main.h"

extern SPI_HandleTypeDef hspi4;
static volatile DSTATUS s_stat = STA_NOINIT;

DSTATUS SD_disk_init(BYTE lun) {
	(void) lun;
	if(SD_Init(&hspi4, SD_CS_GPIO_Port, SD_CS_Pin) == SD_OK) {
		s_stat &= ~STA_NOINIT;
	}
	else {
		s_stat = STA_NOINIT;
	}

	return s_stat;
}

DSTATUS SD_disk_status(BYTE lun) {
	(void)lun;
	return s_stat;
}

DRESULT SD_disk_read(BYTE lun, BYTE *buff, DWORD sector, UINT count) {
	(void)lun;
	if(s_stat & STA_NOINIT) {
		return RES_NOTRDY;
	}

	for(UINT i = 0; i < count; i++) {
		if(SD_ReadBlock(sector + i, buff + i * 512) != SD_OK) {
			return RES_ERROR;
		}
	}

	return RES_OK;
}

DRESULT SD_disk_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count) {
	(void)lun;
	if(s_stat & STA_NOINIT) {
		return RES_NOTRDY;
	}

	for(UINT i = 0; i < count; i++) {
		if(SD_WriteBlock(sector + i, buff + i * 512) != SD_OK) {
			return RES_ERROR;
		}
	}

	return RES_OK;
}

DRESULT SD_disk_ioctl(BYTE lun, BYTE cmd, void *buff) {
	(void)lun;
	switch(cmd) {
		case CTRL_SYNC:
			return RES_OK;

		case GET_SECTOR_SIZE:
			*(WORD *)buff = 512;
			return RES_OK;

		case GET_BLOCK_SIZE:
			*(DWORD *)buff = 1;
			return RES_OK;

		case GET_SECTOR_COUNT:
			// TODO
			// parse CSD (CMD9)

			*(DWORD *)buff = 8000000UL;
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
