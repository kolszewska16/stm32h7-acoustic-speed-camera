#ifndef INC_SD_SPI_H_
#define INC_SD_SPI_H_

#include <stdint.h>
#include "stm32h7xx_hal.h"

typedef enum {
	SD_OK,
	SD_ERROR_TIMEOUT,
	SD_ERROR_CMD,
	SD_ERROR_NO_CARD,
	SD_ERROR_WRITE,
	SD_ERROR_READ,
} sdStatus_t;

typedef enum {
	SD_TYPE_UNKNOWN,
	SD_TYPE_SDSC,
	SD_TYPE_SDHC_SDXC
} sdCardType_t;

sdStatus_t SD_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);

sdStatus_t SD_ReadBlock(uint32_t block_addr, uint8_t *buf);
sdStatus_t SD_WriteBlock(uint32_t block_addr, const uint8_t *buf);

sdCardType_t SD_GetCardType(void);
uint64_t SD_GetCardSizeBytes(void);

#endif /* INC_SD_SPI_H_ */
