/**
 * @file sd_spi.h
 *
 * @brief Low-level SD card driver operating over SPI interface.
 *
 * @details Implements the initialization sequence and single-block
 * 			read/write operations ad defined by the SD Physical Layer
 * 			Simplified Specification (SPI mode). Supports both standard
 * 			capacity (SDSC) and high/extended capacity (SDHC/SDXC),
 * 			automatically selecting the correct block addressing scheme.
 *
 * 			The SPI peripheral must be configured for Mode 3 (CPOL=1,
 * 			CPHA=1) and clocked below 400 kHz prior to calling SD_Init().
 * 			The caller is responsible for raising the SPI clock to its
 * 			target frequency after a successful initialization.
 */

#ifndef INC_SD_SPI_H_
#define INC_SD_SPI_H_

#include <stdint.h>
#include "stm32h7xx_hal.h"

/**
 * @brief Status codes returned by the SD SPI driver.
 */
typedef enum {
	SD_OK,				/**< Operation completed successfully. */
	SD_ERROR,			/**< Generic/invalid-argument error (e.g. NULL handle or buffer). */
	SD_ERROR_TIMEOUT,	/**< Card did not respond within the expected time window. */
	SD_ERROR_CMD,		/**< Card returned unexpected/invalid response to a command. */
	SD_ERROR_NO_CARD,	/**< No card detected, or CMD0 failed to bring the card to idle state. */
	SD_ERROR_WRITE,		/**< Block write operation failed. */
	SD_ERROR_READ,		/**< Block read operation failed. */
} sdStatus_t;

/**
 * @brief Detected SD card capacity class.
 *
 * @details Determines the addressing scheme used by
 * 			SD_ReadBlock()/SD_WriteBlock(): SDSC cards are addressed by
 * 			byte offset, while SDHC/SDXC cards are addressed directly
 * 			by block number.
 */
typedef enum {
	SD_TYPE_UNKNOWN,	/**< Card type not yet determined (before SD_Init() succeeds). */
	SD_TYPE_SDSC,		/**< Standard Capacity card (byte addressing). */
	SD_TYPE_SDHC_SDXC,	/**< High/Extend Capacity card (block addressing). */
} sdCardType_t;

/**
 * @brief Handle describing a single SD card instance and its SPI binding.
 *
 * @details Groups together the SPI peripheral handle, chip-select pin
 * 			assignment, and the card type detected by SD_Init(), so that
 * 			the driver supports more than one card/SPI instance without
 * 			relying on module-level global state.
 */
typedef struct {
	SPI_HandleTypeDef *hspi;	/**< SPI peripheral handle used for all transactions with this card. */

	GPIO_TypeDef *cs_port;		/**< GPIO port of the chip-select (CS) pin. */
	uint16_t cs_pin;			/**< GPIO pin number of the chip-select (CS) pin. */

	sdCardType_t card_type;		/**< Card type detected during SD_Init(); SD_TYPE_UNKNOWN until then. */
} sdCard_HandleTypeDef;

/**
 * @brief Initializes an SD card over SPI.
 *
 * @details Performs the SPI-mode initialization sequence: sends at
 * 			least 74 clock cycles with CS deasserted, then executes
 * 			CMD0, CMD8, ACMD41 (preceded by CMD55) and CMD58 to bring
 * 			the card out of the idle state and determine its capacity
 * 			class. For SDSC cards, the block length is additionally
 * 			fixed to 512 bytes via CMD16.
 *
 * @param[in, out] sd	Pointer to the SD card handle. Must not be NULL
 *
 * @return	SD_OK on success; SD_ERROR_NO_CARD, SD_ERROR_CMD, or SD_ERROR_TIMEOUT
 * 			on failure, depending on which stage of the sequence failed.
 *
 * @note This function must be called before any other function in this
 * 		 driver. It is not reentrant and not thread-safe; if called from
 * 		 multiple RTOS tasks, external synchronization is required.
 */
sdStatus_t SD_Init(sdCard_HandleTypeDef *sd);

/**
 * @brief Reads a single 512-byte block from the card.
 *
 * @details Sends the read command, waits for the data start token
 * 			(0xFE), then reads the 512-byte payload before returning.
 *
 * @param[in] sd			Pointer to the SD card handle. Must not be NULL.
 * @param[in] block_addr	Block address. Interpreted as a byte offset for
 * 							SDSC cards, or directly as a block number for
 * 							SDHC/SDXC cards (handled internally based on the
 * 							type detected by SD_Init()).
 * @param[out] buf			Destination buffer; must be at least 512 bytes.
 *
 * @return SD_OK on success, SD_ERROR_READ or SD_ERROR_TIMEOUT on failure.
 *
 * @pre SD_Init() must have returned SD_OK prior to calling this function.
 */
sdStatus_t SD_ReadBlock(const sdCard_HandleTypeDef *sd, uint32_t block_addr, uint8_t *buf);

/**
 * @brief Read a single 512-byte block from the card using DMA.
 *
 * @details Sends the read command, waits for the data start token
 * 			(0xFE), then transfers the 512-byte payload via DMA
 * 			(see spi_block_txrx_dma()). The calling task blocks on
 * 			a semaphore until the DMA transfers completes.
 *
 * @param[in] sd			Pointer to the SD card handle. Must not be NULL.
 * @param[in] block_addr	Block address. Interpreted as a byte offset for
 * 							SDSC cards, or directly as a block number for
 * 							SDHC/SDXC cards (handled internally based on the
 * 							type detected by SD_Init()).
 * @param[out] buf			Destination buffer; must be at least 512 bytes.
 *
 * @return SD_OK on success, SD_ERROR_READ or SD_ERROR_TIMEOUT on failure.
 *
 * @pre SD_Init() must have returned SD_OK prior to calling this function.
 *
 * @note Must be called from an RTOS task context (uses osSemaphoreAcquire
 * 		 internally via spi_txrx_dma()), not from ISR.
 */
sdStatus_t SD_ReadBlock_DMA(const sdCard_HandleTypeDef *sd, uint32_t block_addr, uint8_t *buf);

/**
 * @brief Writes a single 512-byte block to the card.
 *
 * @details Sends the write command, the data start token (0xFE),
 * 			the 512-byte payload, and waits for the card to signal
 * 			completion of the internal flash programming cycle
 * 			before returning.
 *
 * @param[in] sd			Pointer to the SD card handle. Must not be NULL.
 * @param[in] block_addr	Block address. Interpreted as a byte offset for
 * 							SDSC cards, or directly as a block number for
 * 							SDHC/SDXC cards (handled internally based on the
 * 							type detected by SD_Init()).
 * @param[in] buf			Source buffer; must contain at least 512 bytes.
 *
 * @return SD_OK on success, SD_ERROR_WRITE or SD_ERROR_TIMEOUT on failure.
 *
 * @pre SD_Init() must have returned SD_OK prior to calling this function.
 */
sdStatus_t SD_WriteBlock(const sdCard_HandleTypeDef *sd, uint32_t block_addr, const uint8_t *buf);

/**
 * @brief Writes a single 512-byte block to the card using DMA.
 *
 * @details Sends the write command, the data start token (0xFE),
 * 			the 512-byte payload via DMA, and waits for the card to
 * 			signal completion of the internal flash programming cycle
 * 			before returning.
 *
 * @param[in] sd			Pointer to the SD card handle. Must not be NULL.
 * @param[in] block_addr	Block address. Interpreted as a byte offset for
 * 							SDSC cards, or directly as a block number for
 * 							SDHC/SDXC cards (handled internally based on the
 * 							type detected by SD_Init()).
 * @param[in] buf			Source buffer; must contain at least 512 bytes.
 *
 * @return SD_OK on success, SD_ERROR_WRITE or SD_ERROR_TIMEOUT on failure.
 *
 * @pre SD_Init() must have returned SD_OK prior to calling this function.
 *
 * @note Must be called from an RTOS task context (uses osSemaphoreAcquire
 * 		 internally via spi_txrx_dma()), not from ISR.
 */
sdStatus_t SD_WriteBlock_DMA(const sdCard_HandleTypeDef *sd, uint32_t block_addr, const uint8_t *buf);

/**
 * @brief Returns the capacity class of the currently initialized card.
 *
 * @param[in] sd	Pointer to the SD card handle. Must not be NULL.
 *
 * @return	The detected sdCardType_t, or SD_TYPE_UNKNOWN if SD_Init() has
 * 			not been called or did not succeed.
 */
sdCardType_t SD_GetCardType(const sdCard_HandleTypeDef *sd);

/**
 * @brief Returns the total capacity of the card in bytes.
 *
 * @return Card capacity in bytes.
 *
 * @todo Not yet implemented; requires parsing the CSD register via CMD9.
 * 		 Currently always returns 0.
 */
uint64_t SD_GetCardSizeBytes(void);

#endif /* INC_SD_SPI_H_ */
