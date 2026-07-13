#include "oled.h"

#include <stdio.h>

#include "bsp_config.h"
#include "i2c.h"
#include "spi.h"
#include "ssd1306.h"

static SSD1306_t g_display;

static Status_t OLED_Write(bool dataMode, const uint8_t *data, uint16_t length)
{
#if OLED_USE_SPI
    if (dataMode) { DL_GPIO_setPins(OLED_SPI_DC_PORT, OLED_SPI_DC_PIN); }
    else { DL_GPIO_clearPins(OLED_SPI_DC_PORT, OLED_SPI_DC_PIN); }
    DL_GPIO_clearPins(OLED_SPI_CS_PORT, OLED_SPI_CS_PIN);
    Status_t status = SPI_Write(data, length);
    DL_GPIO_setPins(OLED_SPI_CS_PORT, OLED_SPI_CS_PIN);
    return status;
#else
    return I2C_WriteRegister(OLED_I2C_ADDRESS, dataMode ? 0x40U : 0x00U,
                             data, length);
#endif
}

Status_t OLED_Init(void)
{
#if OLED_USE_SPI
    Status_t status = SPI_Init();
    if (status != STATUS_OK) { return status; }
    DL_GPIO_clearPins(OLED_SPI_RESET_PORT, OLED_SPI_RESET_PIN);
    delay_cycles(32000U);
    DL_GPIO_setPins(OLED_SPI_RESET_PORT, OLED_SPI_RESET_PIN);
#else
    Status_t status = I2C_Init();
    if (status != STATUS_OK) { return status; }
#endif
    return SSD1306_Init(&g_display, OLED_Write);
}

void OLED_Clear(void) { SSD1306_Clear(&g_display); }

Status_t OLED_ShowString(uint8_t x, uint8_t y, const char *text)
{
    if (text == NULL) { return STATUS_INVALID_PARAM; }
    SSD1306_DrawString(&g_display, x, y, text);
    return STATUS_OK;
}

Status_t OLED_ShowInt(uint8_t x, uint8_t y, int32_t value)
{
    char buffer[16];
    (void)snprintf(buffer, sizeof(buffer), "%ld", (long)value);
    return OLED_ShowString(x, y, buffer);
}

Status_t OLED_ShowFloat(uint8_t x, uint8_t y, float value, uint8_t decimals)
{
    char buffer[20];
    if (decimals > 3U) { return STATUS_INVALID_PARAM; }
    (void)snprintf(buffer, sizeof(buffer), "%.*f", (int)decimals, (double)value);
    return OLED_ShowString(x, y, buffer);
}

Status_t OLED_Refresh(void) { return SSD1306_Refresh(&g_display); }
