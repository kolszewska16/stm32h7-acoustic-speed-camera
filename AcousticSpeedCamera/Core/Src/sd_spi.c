#include "sd_spi.h"
#include <string.h>

#define SD_TIMEOUT_MS 200
#define SD_INIT_CLK_TRIES 100

#define CMD0 0		// GO_IDLE_STATE
#define CMD1 1		// SEND_OP_COND
#define CMD6 6		// SWITCH_FUNC
#define CMD8 8		// SEND_IF_COND
#define CMD9 9		// SEND_CSD
#define CMD12 12	// STOP_TRANSMISSION
#define CMD13 13	// SEND_STATUS
#define CMD16 16	// SET_BLOCKLEN
#define CMD17 17	// READ_SINGLE_BLOCK
#define CMD24 24	// WRITE_BLOCK
#define CMD55 55	// APP_CMD
#define CMD58 58	// READ_OCR
#define CMD59 59	// CRC_ON_OFF
#define ACMD41 41	// SD_SEND_OP_COND

static SPI_HandleTypeDef *s_hspi;
static GPIO_TypeDef *s_cs_port;
static uint16_t s_cs_pin;
static sdCardType_t s_card_type = SD_TYPE_UNKNOWN;

static void CS_LOW(void) {
	HAL_GPIO_WritePin(s_cs_port, s_cs_pin, GPIO_PIN_RESET);
}

static void CS_HIGH(void) {
	HAL_GPIO_WritePin(s_cs_port, s_cs_pin, GPIO_PIN_SET);
}

static uint8_t spi_txrx(uint8_t data) {
	uint8_t rx = 0xFF;
	HAL_SPI_TransmitReceive(s_hspi, &data, &rx, 1, HAL_MAX_DELAY);
	return rx;
}

static void spi_clk_bytes(uint8_t n) {
	uint8_t dummy = 0xFF;
	uint8_t rx;

	for(uint8_t i = 0; i < n; i++) {
		HAL_SPI_TransmitReceive(s_hspi, &dummy, &rx, 1, HAL_MAX_DELAY);
	}
}

static uint8_t sd_wait_response(void) {
	uint32_t start = HAL_GetTick();
	uint8_t r = 0xFF;

	while((HAL_GetTick() - start) < SD_TIMEOUT_MS) {
		r = spi_txrx(0xFF);
		if(r != 0xFF) {
			return r;
		}
	}

	return 0xFF; // timeout
}

static sdStatus_t sd_wait_no_busy(void) {
	uint32_t start = HAL_GetTick();

	while(spi_txrx(0xFF) != 0xFF) {
		if((HAL_GetTick() - start) > 500) {
			return SD_ERROR_TIMEOUT;
		}
	}

	return SD_OK;
}

static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
	uint8_t frame[6];

	frame[0] = 0x40 | (cmd & 0x3F);
	frame[1] = (arg >> 24) & 0xFF;
	frame[2] = (arg >> 16) & 0xFF;
	frame[3] = (arg >> 8) & 0xFF;
	frame[4] = arg & 0xFF;
	frame[5] = crc;

	spi_clk_bytes(1);
	CS_LOW();

	for(int i = 0; i < 6; i++) {
		spi_txrx(frame[i]);
	}

	uint8_t r1 = sd_wait_response();
	return r1;
}

static void sd_end_cmd(void) {
	spi_txrx(0xFF);
	CS_HIGH();
	spi_clk_bytes(1);
}

sdStatus_t SD_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) {
	s_hspi = hspi;
	s_cs_port = cs_port;
	s_cs_pin = cs_pin;
	s_card_type = SD_TYPE_UNKNOWN;

	HAL_Delay(200);

	CS_HIGH();
	spi_clk_bytes(20);

	// CMD0 - idle state, waiting for R1 == 0x01
	uint8_t r1 = 0xFF;
	int tries = SD_INIT_CLK_TRIES;
	while(r1 != 0x01 && tries > 0) {
		r1 = sd_send_cmd(CMD0, 0, 0x95);
		sd_end_cmd();
		tries--;
	}

	if(r1 != 0x01) {
		return SD_ERROR_NO_CARD;
	}

	// CMD59 - turning off CRC check
//	r1 = sd_send_cmd(CMD59, 0, 0x01);
//	sd_end_cmd();

	// CMD8 - checking version of the interface (SDv2 or SDv1/MMC)
	r1 = sd_send_cmd(CMD8, 0x1AA, 0x87);
	uint8_t ocr_reply[4] = {0};
	int is_v2 = (r1 == 0x01);
	if(is_v2) {
		for(int i = 0; i < 4; i++) {
			ocr_reply[i] = spi_txrx(0xFF);
		}
	}
	sd_end_cmd();

	if(is_v2 && (ocr_reply[2] != 0x01 || ocr_reply[3] != 0xAA)) {
		return SD_ERROR_CMD;
	}

	uint32_t start = HAL_GetTick();
	while(r1 != 0x00) {
		r1 = sd_send_cmd(CMD55, 0, 0x01);
		sd_end_cmd();
		r1 = sd_send_cmd(ACMD41, is_v2 ? (1UL << 30) : 0, 0x01); // HCS bit for SDHC
		sd_end_cmd();
		if((HAL_GetTick() - start) > 1000) {
			return SD_ERROR_TIMEOUT;
		}
	}

	// CMD58 - checking type of the card (CSS bit)
	s_card_type = SD_TYPE_SDSC;
	if(is_v2) {
		r1 = sd_send_cmd(CMD58, 0, 0x01);
		uint8_t ocr[4];
		for(int i = 0; i < 4; i++) {
			ocr[i] = spi_txrx(0xFF);
		}
		sd_end_cmd();
		if(r1 == 0x00 && (ocr[0] & 0x40)) {
			s_card_type = SD_TYPE_SDHC_SDXC;
		}
	}

	// blocklen for SDSC
	if(s_card_type == SD_TYPE_SDSC) {
		r1 = sd_send_cmd(CMD16, 512, 0x01);
		sd_end_cmd();
		if(r1 != 0x00) {
			return SD_ERROR_CMD;
		}
	}

	return SD_OK;
}

sdStatus_t SD_ReadBlock(uint32_t block_addr, uint8_t *buf) {
	// SDSC -> byte addressing, SDHC/SDXC -> block addressing
	uint32_t addr = (s_card_type == SD_TYPE_SDHC_SDXC) ? block_addr : (block_addr * 512);

	uint8_t r1 = sd_send_cmd(CMD17, addr, 0x01);
	if(r1 != 0x00) {
		sd_end_cmd();
		return SD_ERROR_READ;
	}

	// waiting for 0xFE (data start token)
	uint32_t start = HAL_GetTick();
	uint8_t token = 0x00;
	while(token != 0xFE) {
		token = spi_txrx(0xFF);
		if((HAL_GetTick() - start) > SD_TIMEOUT_MS) {
			sd_end_cmd();
			return SD_ERROR_TIMEOUT;
		}
	}

	for(int i = 0; i < 512; i++) {
		buf[i] = spi_txrx(0xFF);
	}

	spi_txrx(0xFF);
	spi_txrx(0xFF);
	sd_end_cmd();

	return SD_OK;
}

sdStatus_t SD_WriteBlock(uint32_t block_addr, const uint8_t *buf) {
	uint32_t addr = (s_card_type == SD_TYPE_SDHC_SDXC) ? block_addr : (block_addr * 512);

	uint8_t r1 = sd_send_cmd(CMD24, addr, 0x01);
	if(r1 != 0x00) {
		sd_end_cmd();
		return SD_ERROR_WRITE;
	}

	spi_txrx(0xFF);
	spi_txrx(0xFE); // data start token

	for(int i = 0; i < 512; i++) {
		spi_txrx(buf[i]);
	}

	spi_txrx(0xFF);
	spi_txrx(0xFF);

	uint8_t data_resp = spi_txrx(0xFF);
	if((data_resp & 0x1F) != 0x05) {
		sd_end_cmd();
		return SD_ERROR_WRITE;
	}

	if(sd_wait_no_busy() != SD_OK) {
		sd_end_cmd();
		return SD_ERROR_TIMEOUT;
	}

	sd_end_cmd();
	return SD_OK;
}

sdCardType_t SD_GetCardType(void) {
	return s_card_type;
}

uint64_t SD_GetCardSizeBytes(void) {
	// TODO
	// parse CSD from CMD9

	return 0;
}
