/**
 * @file sd_spi.c
 *
 * @brief Implementation of the SD card SPI driver.
 *
 * @details See sd_spi.h for the public API documentation. This file
 * 			contains the low-level SPI transaction primitives and the
 * 			SD command framing logic, implemented in accordance with the
 * 			SD Physical Layer Simplified Specification (SPI mode).
 */

#include "sd_spi.h"
#include <stdint.h>
#include <string.h>
#include "cmsis_os.h"
#include "os_objects.h"

#define SD_TIMEOUT_MS 200		/**< Response wait timeout, in milliseconds, for command responses (R1) */
#define SD_INIT_CLK_TRIES 100	/**< Maximum number of CMD0 retries during the initialization. */

#define CMD0	0	/**< GO_IDLE_STATE - resets the card and selects SPI mode. */
#define CMD8	8	/**< SEND_IF_COND - queries interface version and voltage range. */
#define CMD9	9	/**< SEND_CSD - reads the Card-Specific Data register. */
#define CMD12	12	/**< STOP_TRANSMISSION - terminates a multi-block read. */
#define CMD16	16	/**< SET_BLOCKLEN - sets the block length. */
#define CMD17	17	/**< READ_SINGLE_BLOCK - reads one 512-byte block. */
#define CMD24	24	/**< WRITE_BLOCK - writes one 512-byte block. */
#define CMD55	55	/**< APP_CMD - signal that the next command is an ACMD. */
#define CMD58	58	/**< READ_OCR - reads the Operation Conditions Register. */
#define ACMD41	41	/**< SD_SEND_OP_COND - initiates the card initialization process. */

static SPI_HandleTypeDef *s_hspi;					/**< SPI peripheral handle used for all transactions. */
static GPIO_TypeDef *s_cs_port;						/**< GPIO port of the chip-select (CS) pin. */
static uint16_t s_cs_pin;							/**< GPIO pin number of the chip-select (CS) pin. */
static sdCardType_t s_card_type = SD_TYPE_UNKNOWN;	/**< Card type detected during SD_Init(). */

/**
 * @brief Assert the chip-select line (drives CS low, selects the card).
 */
static void CS_LOW(void) {
	HAL_GPIO_WritePin(s_cs_port, s_cs_pin, GPIO_PIN_RESET);
}

/**
 * @brief Deasserts the chip-select line (drives CS high, deselects the card).
 */
static void CS_HIGH(void) {
	HAL_GPIO_WritePin(s_cs_port, s_cs_pin, GPIO_PIN_SET);
}

/**
 * @brief Exchanges a single byte over SPI (blocking, full-duplex).
 *
 * On a HAL transfer error, forces the SPI peripheral state back to
 * HAL_SPI_STATE_READY to avoid leaving the driver stuck in a busy state
 * after a failed transaction.
 *
 * @param[in] data	Byte to transmit on MOSI.
 *
 * @return	Byte simultaneously received on MISO, or 0xFF if the HAL
 * 			transfer failed.
 */
static uint8_t spi_txrx(uint8_t data) {
	uint8_t rx = 0xFF;

	if(HAL_SPI_TransmitReceive(s_hspi, &data, &rx, 1, HAL_MAX_DELAY) != HAL_OK) {
		s_hspi->State = HAL_SPI_STATE_READY;
		return 0xFF;
	}

	return rx;
}

/**
 * @brief Exchanges a block of data over SPI using DMA.
 *
 * Starts a DMA-based full-duplex transfer and blocks the calling RTOS
 * task on a binary semaphore until the transfer completes (signaled from
 * HAL_SPI_TxRxCpltCallback()) or the timeout expires. On failure to start
 * the transfer, or on a semaphore timeout, aborts any in-progress DMA
 * transfer via HAL_SPI_Abort() to leave the peripheral in a consistent
 * state for the next call.
 *
 * @param[in] tx	Buffer to transmit; must remain valid for the duration
 * 					of the transfer.
 * @param[out] rx	Buffer to receive into; must be at least @param len bytes.
 * @param[in] len	Number of bytes to exchange.
 *
 * @return	SD_OK on success, SD_ERROR_READ if the DMA transfer could not
 * 			be started, SD_ERROR_TIMEOUT if it did not complete in time.
 *
 * @note Must be called from an RTOS task context, not from ISR.
 */
static sdStatus_t spi_block_txrx_dma(const uint8_t *tx, uint8_t *rx, uint16_t len) {
	if(HAL_SPI_TransmitReceive_DMA(s_hspi, (uint8_t*)tx, rx, len) != HAL_OK) {
		HAL_SPI_Abort(s_hspi);
		return SD_ERROR_READ;
	}

	if(osSemaphoreAcquire(s_spi_dma_sem, pdMS_TO_TICKS(200)) != osOK) {
		HAL_SPI_Abort(s_hspi);
		return SD_ERROR_TIMEOUT;
	}

	return SD_OK;
}

/**
 * @brief Generates clock pulses without conveying meaningful data.
 *
 * Send @param n bytes of 0xFF, used to satisfy the SD specification's
 * requirements for idle clock cycles (e.g. the 74-cycle power-up sequence,
 * or the mandatory spacing around a command frame).
 *
 * @param[in] n	Number of dummy bytes (8 clock cycles each) to send.
 */
static void spi_clk_bytes(uint8_t n) {
	uint8_t dummy = 0xFF;
	uint8_t rx;

	for(uint8_t i = 0; i < n; i++) {
		HAL_SPI_TransmitReceive(s_hspi, &dummy, &rx, 1, HAL_MAX_DELAY);
	}
}

/**
 * @brief Polls the card until it returns a byte other than 0xFF (R1 token).
 *
 * @return	The received R1 response byte, or 0xFF if SD_TIMEOUT_MS elapses
 * 			without a response.
 */
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

/**
 * @brief Waits until the cart deasserts its internal busy signal.
 *
 * After a block write, the card holds MISO low while programming its
 * internal flash memory. Polls until MISO returns to 0xFF or the
 * 500 ms timeout expires.
 *
 * @return	SD_OK once the card is no longer busy, SD_ERROR_TIMEOUT on
 * 			timeout.
 */
static sdStatus_t sd_wait_no_busy(void) {
	uint32_t start = HAL_GetTick();

	while(spi_txrx(0xFF) != 0xFF) {
		if((HAL_GetTick() - start) > 500) {
			return SD_ERROR_TIMEOUT;
		}
	}

	return SD_OK;
}

/**
 * @brief Builds and transmits a 6-byte SD command frame, then waits for R1.
 *
 * Frame layout: [start bit + command] | arg[31:24] | arg[23:16] |
 * arg[15:8] | arg[7:0] | crc]. Asserts CS for the duration of the
 * transaction; the caller must call sd_end_cmd() afterwards to
 * deassert CS.
 *
 * @param[in] cmd	Command index (0-63, without the start-bit prefix;
 * 					the 0x40 start bit is added internally.
 * @param[in] arg	32-bit command argument.
 * @param[in] crc	CRC byte. Only meaningful for CMD0 and CMD8 under the
 * 					default (CR-disabled) SPI mode; ignored by the card
 * 					for all other commands.
 *
 * @return The R1 response byte (0xFF on timeout).
 */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
	uint8_t frame[6];

	frame[0] = 0x40 | (cmd & 0x3F);
	frame[1] = (arg >> 24) & 0xFF;
	frame[2] = (arg >> 16) & 0xFF;
	frame[3] = (arg >> 8) & 0xFF;
	frame[4] = arg & 0xFF;
	frame[5] = crc;

	CS_LOW();
	spi_clk_bytes(1);

	for(int i = 0; i < 6; i++) {
		spi_txrx(frame[i]);
	}

	uint8_t r1 = sd_wait_response();
	return r1;
}

/**
 * @brief Terminates an SD command transaction, deasserting CS.
 */
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

	uint32_t init_retries = 2000;
	while(r1 != 0x00 && init_retries > 0) {
		r1 = sd_send_cmd(CMD55, 0, 0x01);
		sd_end_cmd();
		r1 = sd_send_cmd(ACMD41, is_v2 ? (1UL << 30) : 0, 0x01); // HCS bit for SDHC
		sd_end_cmd();
		init_retries--;
		HAL_Delay(1);
	}

	if(r1 != 0x00) {
		return SD_ERROR_TIMEOUT;
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

sdStatus_t SD_ReadBlock_DMA(uint32_t block_addr, uint8_t *buf) {
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

	uint8_t tx_dummy[512];
	memset(tx_dummy, 0xFF, 512);
	if(spi_block_txrx_dma(tx_dummy, buf, 512) != SD_OK) {
		sd_end_cmd();
		return SD_ERROR_TIMEOUT;
	}

	spi_txrx(0xFF);
	spi_txrx(0xFF);
	sd_end_cmd();

	return SD_OK;
}

sdStatus_t SD_WriteBlock(uint32_t block_addr, const uint8_t *buf) {
	// SDSC -> byte addressing, SDHC/SDXC -> block addressing
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

sdStatus_t SD_WriteBlock_DMA(uint32_t block_addr, const uint8_t *buf) {
	// SDSC -> byte addressing, SDHC/SDXC -> block addressing
	uint32_t addr = (s_card_type == SD_TYPE_SDHC_SDXC) ? block_addr : (block_addr * 512);

	uint8_t r1 = sd_send_cmd(CMD24, addr, 0x01);
	if(r1 != 0x00) {
		sd_end_cmd();
		return SD_ERROR_WRITE;
	}

	spi_txrx(0xFF);
	spi_txrx(0xFE); // data start token

	uint8_t rx_dummy[512];
	if(spi_block_txrx_dma(buf, rx_dummy, 512) != SD_OK) {
		sd_end_cmd();
		return SD_ERROR_WRITE;
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
