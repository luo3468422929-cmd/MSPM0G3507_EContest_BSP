#ifndef LCD_BITMAP_H
#define LCD_BITMAP_H

#include "common.h"

/**
 * 显示 RGB565 原始图片。
 *
 * 图片按行优先存储，每个像素两个字节，高字节在前；调用者负责保证
 * rgb565 指向 width * height * 2 字节的只读数据。
 */
Status_t LCD_ShowBitmap(uint16_t x, uint16_t y, uint16_t width,
                        uint16_t height, const uint8_t *rgb565);

#endif
