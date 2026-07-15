#include "lcd_bitmap.h"

#include "lcd.h"

Status_t LCD_ShowBitmap(uint16_t x, uint16_t y, uint16_t width,
                        uint16_t height, const uint8_t *rgb565)
{
    uint32_t length;

    if ((rgb565 == NULL) || (width == 0U) || (height == 0U)) {
        return STATUS_INVALID_PARAM;
    }
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT) ||
        (width > (LCD_WIDTH - x)) || (height > (LCD_HEIGHT - y))) {
        return STATUS_INVALID_PARAM;
    }
    length = (uint32_t)width * (uint32_t)height * 2U;
    return LCD_WriteRegion(x, y, width, height, rgb565, length);
}
