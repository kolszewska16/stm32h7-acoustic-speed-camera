#ifndef INC_ARDUCAM_OV2640_H_
#define INC_ARDUCAM_OV2640_H_

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

typedef struct {
    I2C_HandleTypeDef *hi2c;
    SPI_HandleTypeDef *hspi;

    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} ArduCam_HandleTypedef;

typedef enum {
    CAMERA_OK,
    CAMERA_ERROR,
    CAMERA_I2C_ERROR,
    CAMERA_SPI_ERROR,
} cameraStatus_t;

void ArduCam_CS_LOW(const ArduCam_HandleTypedef *cam);
void ArduCam_CS_HIGH(const ArduCam_HandleTypedef *cam);

HAL_StatusTypeDef ArduCam_SPI_write_reg(const ArduCam_HandleTypedef *cam,
		const uint8_t reg, const uint8_t val);
HAL_StatusTypeDef ArduCam_SPI_read_reg(const ArduCam_HandleTypedef *cam,
		const uint8_t reg, uint8_t *val);

HAL_StatusTypeDef ArduCam_I2C_write_byte(const ArduCam_HandleTypedef *cam,
		const uint8_t reg, const uint8_t val);
HAL_StatusTypeDef ArduCam_I2C_write_table(const ArduCam_HandleTypedef *cam,
		const uint8_t table[][2]);
HAL_StatusTypeDef ArduCam_I2C_read(const ArduCam_HandleTypedef *cam,
		const uint8_t reg, uint8_t *val);

cameraStatus_t ArduCam_I2C_ready(const ArduCam_HandleTypedef *cam,
		uint8_t *addr_out);
cameraStatus_t ArduCam_I2C_scan_bus(const ArduCam_HandleTypedef *cam,
    uint8_t *found_addr, const size_t found_addr_max, size_t *found_addr_len);
cameraStatus_t ArduCam_I2C_read_chip(const ArduCam_HandleTypedef *cam,
		uint8_t *pidh, uint8_t *pidl);

cameraStatus_t ArduCam_Arduchip_Reset(const ArduCam_HandleTypedef *cam);
cameraStatus_t ArduCam_Init(const ArduCam_HandleTypedef *cam);

cameraStatus_t ArduCam_ClearFifoFlag(const ArduCam_HandleTypedef *cam);
cameraStatus_t ArduCam_StartCapture(const ArduCam_HandleTypedef *cam);
bool ArduCam_CaptureDone(const ArduCam_HandleTypedef *cam);
uint32_t ArduCam_ReadFifoLength(const ArduCam_HandleTypedef *cam);
cameraStatus_t ArduCam_ReadFifoBurst(const ArduCam_HandleTypedef *cam, uint8_t *buf, uint32_t length);

#endif /* INC_ARDUCAM_OV2640_H_ */
