/**
 * @file lcd_bitmap.c
 * @brief 校验 RGB565 图片区域后转交 LCD_WriteRegion() 直接显示。
 *
 * 所属层：Hardware LCD 扩展。输入图片必须已经转换为高字节在前的 RGB565，
 * 本文件不申请显存、不缩放，也不长期保存调用者指针。
 */
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
    /* 使用 uint32 计算总字节数，覆盖 128×160×2 的完整屏幕图片。 */
    length = (uint32_t)width * (uint32_t)height * 2U;
    return LCD_WriteRegion(x, y, width, height, rgb565, length);
}
