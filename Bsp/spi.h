/**
 * @file spi.h
 * @brief 提供 ST7735S LCD 使用的阻塞式 SPI1 发送通道。
 *
 * 所属层：Bsp 总线层。CS/DC/RES 由 Hardware/lcd.c 控制，本模块只发送
 * 字节并实施有限超时，不解释 LCD 命令。
 */
#ifndef BSP_SPI_H
#define BSP_SPI_H

#include "common.h"

/** @brief 确认 SPI 通道可用；硬件寄存器已由 SYSCFG_DL_init() 配置。 */
Status_t SPI_Init(void);

/**
 * @brief 阻塞发送连续字节，并等待 SPI 最后一个字节移出。
 * @return STATUS_TIMEOUT 表示 FIFO 或 BUSY 等待超限。
 */
Status_t SPI_Write(const uint8_t *data, uint16_t length);

#endif

