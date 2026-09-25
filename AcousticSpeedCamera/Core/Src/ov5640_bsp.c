#include "ov5640_bsp.h"
#include <stddef.h>

static OV5640_HandleTypedef *s_active_cam = NULL;

static int32_t OV5640_IO_Init(void) {
	return 0;
}

static int32_t OV5640_IO_DeInit(void) {
	return 0;
}

static int32_t OV5640_IO_WriteReg(uint16_t dev_addr, uint16_t reg,
		uint8_t *data, uint16_t data_len)
{
	if(s_active_cam == NULL || data == NULL || data_len == 0) {
		return OV5640_ERROR;
	}

	if(HAL_I2C_Mem_Write(s_active_cam->hi2c, dev_addr, reg, I2C_MEMADD_SIZE_16BIT,
			data, data_len, OV5640_I2C_TIMEOUT_MS) != HAL_OK)
	{
		return OV5640_ERROR;
	}

	return OV5640_OK;
}

static int32_t OV5640_IO_ReadReg(uint16_t dev_addr, uint16_t reg,
		uint8_t *data, uint16_t data_len)
{
	if(s_active_cam == NULL || data == NULL || data_len == 0) {
		return OV5640_ERROR;
	}

	if(HAL_I2C_Mem_Read(s_active_cam->hi2c, dev_addr, reg, I2C_MEMADD_SIZE_16BIT,
			data, data_len, OV5640_I2C_TIMEOUT_MS) != HAL_OK)
	{
		return OV5640_ERROR;
	}

	return OV5640_OK;
}

static int32_t OV5640_IO_GetTick(void) {
	return (int32_t)HAL_GetTick();
}

static void OV5640_BSP_HW_Reset() {
	if(s_active_cam == NULL) {
		return;
	}

	HAL_GPIO_WritePin(s_active_cam->pwdn_port, s_active_cam->pwdn_pin, GPIO_PIN_SET);
	HAL_Delay(5);
	HAL_GPIO_WritePin(s_active_cam->rst_port, s_active_cam->rst_pin, GPIO_PIN_RESET);
	HAL_Delay(5);

	HAL_GPIO_WritePin(s_active_cam->pwdn_port, s_active_cam->pwdn_pin, GPIO_PIN_RESET);
	HAL_Delay(5);
	HAL_GPIO_WritePin(s_active_cam->rst_port, s_active_cam->rst_pin, GPIO_PIN_SET);
	HAL_Delay(100);
}

int32_t OV5640_BSP_Init(OV5640_Object_t *pObj, OV5640_HandleTypedef *cam) {
	if(pObj == NULL) {
		return OV5640_ERROR;
	}

	OV5640_IO_t io = {0};
	s_active_cam = cam;
	OV5640_BSP_HW_Reset();

	io.Init = OV5640_IO_Init;
	io.DeInit = OV5640_IO_DeInit;
	io.Address = OV5640_I2C_ADDRESS;
	io.WriteReg = OV5640_IO_WriteReg;
	io.ReadReg = OV5640_IO_ReadReg;
	io.GetTick = OV5640_IO_GetTick;

	if(OV5640_RegisterBusIO(pObj, &io) != OV5640_OK) {
		return OV5640_ERROR;
	}

	return OV5640_OK;
}

int32_t OV5640_BSP_DeInit(OV5640_Object_t *pObj, OV5640_HandleTypedef *cam) {
	if(pObj == NULL || cam == NULL) {
		return OV5640_ERROR;
	}

	HAL_GPIO_WritePin(cam->pwdn_port, cam->pwdn_pin, GPIO_PIN_SET);
	return OV5640_DeInit(pObj);
}
