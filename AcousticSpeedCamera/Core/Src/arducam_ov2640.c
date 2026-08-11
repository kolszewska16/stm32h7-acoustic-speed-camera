#include "arducam_ov2640.h"
#include <stddef.h>
#include "ov2640_regs.h"

#define MAX_I2C_DEVICES 5

void ArduCam_CS_LOW(const ArduCam_HandleTypedef *cam) {
    if(cam == NULL) {
        return;
    }

    HAL_GPIO_WritePin(cam->cs_port, cam->cs_pin, GPIO_PIN_RESET);
}

void ArduCam_CS_HIGH(const ArduCam_HandleTypedef *cam) {
    if(cam == NULL) {
        return;
    }

    HAL_GPIO_WritePin(cam->cs_port, cam->cs_pin, GPIO_PIN_SET);
}

HAL_StatusTypeDef ArduCam_SPI_write_reg(const ArduCam_HandleTypedef *cam, const uint8_t reg, const uint8_t val) {
    if(cam == NULL) {
        return HAL_ERROR;
    }

    uint8_t tx = reg | 0x80;

    ArduCam_CS_LOW(cam);
    if(HAL_SPI_Transmit(cam->hspi, &tx, 1, HAL_MAX_DELAY) != HAL_OK) {
    	ArduCam_CS_HIGH(cam);
    	return HAL_ERROR;
    }

    tx = val;
    if(HAL_SPI_Transmit(cam->hspi, &tx, 1, HAL_MAX_DELAY) != HAL_OK) {
    	ArduCam_CS_HIGH(cam);
    	return HAL_ERROR;
    }

    while(HAL_SPI_GetState(cam->hspi) == HAL_SPI_STATE_BUSY);
    ArduCam_CS_HIGH(cam);

    return HAL_OK;
}

HAL_StatusTypeDef ArduCam_SPI_read_reg(const ArduCam_HandleTypedef *cam, const uint8_t reg, uint8_t *val) {
    if(cam == NULL || val == NULL) {
        return HAL_ERROR;
    }

    uint8_t tx = reg & 0x7F;
    uint8_t empty = 0x00;
    uint8_t rx = 0x00;

    ArduCam_CS_LOW(cam);
    if(HAL_SPI_TransmitReceive(cam->hspi, &tx, &rx, 1, HAL_MAX_DELAY) != HAL_OK) {
    	ArduCam_CS_HIGH(cam);
    	return HAL_ERROR;
    }

    if(HAL_SPI_TransmitReceive(cam->hspi, &empty, &rx, 1, HAL_MAX_DELAY) != HAL_OK) {
    	ArduCam_CS_HIGH(cam);
    	return HAL_ERROR;
    }
    ArduCam_CS_HIGH(cam);

    *val = rx;
    return HAL_OK;
}

HAL_StatusTypeDef ArduCam_I2C_write_byte(const ArduCam_HandleTypedef *cam, const uint8_t reg, const uint8_t val) {
    if(cam == NULL) {
        return HAL_ERROR;
    }

    uint8_t tx[2] = {reg, val};
    if(HAL_I2C_Master_Transmit(cam->hi2c, OV2640_I2C_ADDR << 1, (uint8_t*)tx, sizeof(tx), HAL_MAX_DELAY) != HAL_OK) {
    	return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef ArduCam_I2C_write_table(const ArduCam_HandleTypedef *cam, const uint8_t table[][2]) {
    if(cam == NULL || table == NULL) {
        return HAL_ERROR;
    }

    uint16_t i = 0;
    while(1) {
    	uint8_t reg = table[i][0];
    	uint8_t val = table[i][1];
    	if(reg == 0xFF && val == 0xFF) {
    		break;
    	}

    	if(ArduCam_I2C_write_byte(cam, reg, val) != HAL_OK) {
    		return HAL_ERROR;
    	}

    	i++;
    }

    return HAL_OK;
}

HAL_StatusTypeDef ArduCam_I2C_read(const ArduCam_HandleTypedef *cam, const uint8_t reg, uint8_t *val) {
    if(cam == NULL || val == NULL) {
        return HAL_ERROR;
    }

    uint8_t tx[1] = {reg};
    uint8_t rx[1] = {0};
    if(HAL_I2C_Master_Transmit(cam->hi2c, OV2640_I2C_ADDR << 1, (uint8_t*)tx, sizeof(tx), HAL_MAX_DELAY) != HAL_OK) {
    	return HAL_ERROR;
    }

    rx[0] = 0x00;
    if(HAL_I2C_Master_Receive(cam->hi2c, OV2640_I2C_ADDR << 1, (uint8_t*)rx, sizeof(rx), HAL_MAX_DELAY) != 0) {
    	return HAL_ERROR;
    }

    *val = rx[0];
    return HAL_OK;
}

cameraStatus_t ArduCam_I2C_ready(const ArduCam_HandleTypedef *cam, uint8_t *addr_out) {
    if(cam == NULL || addr_out == NULL) {
        return CAMERA_ERROR;
    }

    if(HAL_I2C_IsDeviceReady(cam->hi2c, OV2640_I2C_ADDR << 1, 10, 100) == HAL_OK) {
        *addr_out = OV2640_I2C_ADDR;
        return CAMERA_OK;
    }

    return CAMERA_I2C_ERROR;
}

cameraStatus_t ArduCam_I2C_scan_bus(const ArduCam_HandleTypedef *cam,
    uint8_t *found_addr, const size_t found_addr_max, size_t *found_addr_len)
{
    if(cam == NULL || found_addr == NULL || found_addr_max <= 0) {
        return CAMERA_ERROR;
    }

    int dev_count = 0;
    for(uint8_t addr = 1; addr < 128; addr++) {
        if(HAL_I2C_IsDeviceReady(cam->hi2c, addr << 1, 1, 100) == HAL_OK) {
            if(dev_count < found_addr_max) {
                found_addr[dev_count] = addr;
            }
            dev_count++;
        }
    }

    *found_addr_len = dev_count;
    return CAMERA_OK;
}

cameraStatus_t ArduCam_I2C_read_chip(const ArduCam_HandleTypedef *cam, uint8_t *pidh, uint8_t *pidl) {
    if(cam == NULL || pidh == NULL || pidl == NULL) {
        return CAMERA_ERROR;
    }

    // register bank select
    uint8_t reg = OV2640_BANK_SELECT;
    uint8_t bank_select = 0x01;
    if(ArduCam_I2C_write_byte(cam, reg, bank_select) != HAL_OK) {
    	return CAMERA_I2C_ERROR;
    }

    // PIDH reading (0x0A)
    reg = OV2640_PIDH;
    if(ArduCam_I2C_read(cam, reg, pidh) != HAL_OK) {
    	return CAMERA_I2C_ERROR;
    }

    // PIDL reading (0x0B)
    reg = OV2640_PIDL;
    if(ArduCam_I2C_read(cam, reg, pidl) != HAL_OK) {
    	return CAMERA_I2C_ERROR;
    }

    return CAMERA_OK;
}

cameraStatus_t ArduCam_Arduchip_Reset(const ArduCam_HandleTypedef *cam) {
	if(cam == NULL) {
		return CAMERA_ERROR;
	}

	if(ArduCam_SPI_write_reg(cam, 0x07, 0x80) != HAL_OK) {
		return CAMERA_SPI_ERROR;
	}
	HAL_Delay(100);
	if(ArduCam_SPI_write_reg(cam, 0x07, 0x00) != HAL_OK) {
		return CAMERA_SPI_ERROR;
	}

	HAL_Delay(100);
	return CAMERA_OK;
}

cameraStatus_t ArduCam_Init(const ArduCam_HandleTypedef *cam) {
    if(cam == NULL) {
        return CAMERA_ERROR;
    }

    if(ArduCam_I2C_write_byte(cam, OV2640_BANK_SELECT, 0x01) != HAL_OK) {
    	return CAMERA_I2C_ERROR;
    }
    if(ArduCam_I2C_write_byte(cam, OV2640_COMMON_CTRL7, 0x80) != HAL_OK) {
    	return CAMERA_I2C_ERROR;
    }
    HAL_Delay(100);

    if(ArduCam_I2C_write_table(cam, OV2640_JPEG_INIT) != HAL_OK) {
    	return CAMERA_I2C_ERROR;
    }
    if(ArduCam_I2C_write_table(cam, OV2640_YUV422) != HAL_OK) {
    	return CAMERA_I2C_ERROR;
    }
    if(ArduCam_I2C_write_table(cam, OV2640_JPEG) != HAL_OK) {
    	return CAMERA_I2C_ERROR;
    }

    if(ArduCam_I2C_write_byte(cam, OV2640_BANK_SELECT, 0x01) != HAL_OK) {
    	return CAMERA_I2C_ERROR;
    }
    if(ArduCam_I2C_write_byte(cam, OV2640_COMMON_CTRL10, 0x00) != HAL_OK) {
    	return CAMERA_I2C_ERROR;
    }

    if(ArduCam_I2C_write_table(cam, OV2640_320x240_JPEG) != HAL_OK) {
    	return CAMERA_I2C_ERROR;
    }


    ArduCam_CS_HIGH(cam);
    ArduCam_Arduchip_Reset(cam);

    uint8_t rx = 0x00;
    if(ArduCam_SPI_write_reg(cam, ARDUCHIP_TEST_REG, 0x55) != HAL_OK) {
        return CAMERA_SPI_ERROR;
    }

    // TODO
    // SPI read debug
    // SPI read is not working: rx != 0x55
    if(ArduCam_SPI_read_reg(cam, ARDUCHIP_TEST_REG, &rx) != HAL_OK) {
        return CAMERA_SPI_ERROR;
    }
    if(rx != 0x55) {
        return CAMERA_SPI_ERROR;
    }

    return CAMERA_OK;
}
