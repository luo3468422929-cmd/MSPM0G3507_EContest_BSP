#include "spi.h"

#include "bsp_config.h"

Status_t SPI_Init(void)
{
#if OLED_USE_SPI
    return STATUS_OK;
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

Status_t SPI_Write(const uint8_t *data, uint16_t length)
{
#if OLED_USE_SPI
    if ((data == NULL) || (length == 0U)) { return STATUS_INVALID_PARAM; }
    for (uint16_t index = 0U; index < length; ++index) {
        uint32_t timeout = OLED_SPI_TIMEOUT_LOOPS;
        while (DL_SPI_isBusy(OLED_SPI_INST) && (timeout > 0U)) { timeout--; }
        if (timeout == 0U) { return STATUS_TIMEOUT; }
        DL_SPI_transmitData8(OLED_SPI_INST, data[index]);
    }
    return STATUS_OK;
#else
    (void)data;
    (void)length;
    return STATUS_NOT_SUPPORTED;
#endif
}

