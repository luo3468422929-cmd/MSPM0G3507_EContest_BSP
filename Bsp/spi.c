/**
 * @file spi.c
 * @brief 实现 LCD SPI1 的有限超时阻塞发送。
 *
 * 所属层：Bsp 总线层。SPI 时钟、模式和引脚由 SysConfig 初始化；这里
 * 只管理 TX FIFO 和 BUSY 状态，不控制 LCD 的 CS/DC/RES。
 */
#include "spi.h"

#include "board_pins.h"
#include "user_config.h"

Status_t SPI_Init(void)
{
    /* SYSCFG_DL_init() 已完成寄存器配置，保留接口便于统一板级初始化。 */
    return STATUS_OK;
}

Status_t SPI_Write(const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U)) { return STATUS_INVALID_PARAM; }
    /* 每个字节在 FIFO 有空间后写入，防止覆盖尚未发送的数据。 */
    for (uint16_t index = 0U; index < length; ++index) {
        uint32_t timeout = LCD_SPI_TIMEOUT_LOOPS;
        while (DL_SPI_isTXFIFOFull(PIN_LCD_SPI_INST) && (timeout > 0U)) {
            timeout--;
        }
        if (timeout == 0U) { return STATUS_TIMEOUT; }
        DL_SPI_transmitData8(PIN_LCD_SPI_INST, data[index]);
    }
    {
        /* 返回前等待移位器空闲，调用者随后拉高 CS 才不会截断最后一字节。 */
        uint32_t timeout = LCD_SPI_TIMEOUT_LOOPS;
        while (DL_SPI_isBusy(PIN_LCD_SPI_INST) && (timeout > 0U)) { timeout--; }
        if (timeout == 0U) { return STATUS_TIMEOUT; }
    }
    return STATUS_OK;
}
