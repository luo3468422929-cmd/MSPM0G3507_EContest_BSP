#ifndef BSP_I2C_H
#define BSP_I2C_H

#include "common.h"

Status_t I2C_Init(void);
Status_t I2C_Write(uint8_t address, const uint8_t *data, uint16_t length);
Status_t I2C_WriteRegister(uint8_t address, uint8_t reg,
                           const uint8_t *data, uint16_t length);

#endif

