#ifndef INC_SD_DRIVER_H_
#define INC_SD_DRIVER_H_

#include "ff_gen_drv.h"
#include "main.h"

extern SPI_HandleTypeDef hspi4;

DSTATUS SD_disk_init(BYTE pdrv);
DSTATUS SD_disk_status(BYTE pdrv);
DRESULT SD_disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
DRESULT SD_disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
DRESULT SD_disk_ioctl(BYTE pdrv, BYTE cmd, void *buff);


#endif /* INC_SD_DRIVER_H_ */
