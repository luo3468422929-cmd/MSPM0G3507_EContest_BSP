#ifndef BSP_SPI_H
#define BSP_SPI_H

#include "common.h"

Status_t SPI_Init(void);
Status_t SPI_Write(const uint8_t *data, uint16_t length);

#endif

