#include "spi.h"

#include "board_pins.h"
#include "user_config.h"

Status_t SPI_Init(void)
{
    return STATUS_OK;
}

Status_t SPI_Write(const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U)) { return STATUS_INVALID_PARAM; }
    for (uint16_t index = 0U; index < length; ++index) {
        uint32_t timeout = LCD_SPI_TIMEOUT_LOOPS;
        while (DL_SPI_isTXFIFOFull(PIN_LCD_SPI_INST) && (timeout > 0U)) {
            timeout--;
        }
        if (timeout == 0U) { return STATUS_TIMEOUT; }
        DL_SPI_transmitData8(PIN_LCD_SPI_INST, data[index]);
    }
    {
        uint32_t timeout = LCD_SPI_TIMEOUT_LOOPS;
        while (DL_SPI_isBusy(PIN_LCD_SPI_INST) && (timeout > 0U)) { timeout--; }
        if (timeout == 0U) { return STATUS_TIMEOUT; }
    }
    return STATUS_OK;
}
