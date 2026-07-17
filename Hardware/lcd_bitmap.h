/**
 * @file lcd_bitmap.h
 * @brief 定义把行优先 RGB565 原始图片直接写入 LCD 的接口。
 *
 * 所属层：Hardware LCD 扩展。不复制图片数据，不进行缩放、透明或格式转换；
 * 大图片会阻塞 SPI，运行小车时应控制刷新频率。
 */
#ifndef LCD_BITMAP_H
#define LCD_BITMAP_H

#include "common.h"

/**
 * @brief 显示 RGB565 原始图片。
 *
 * 图片按行优先存储，每个像素两个字节，高字节在前；调用者负责保证
 * rgb565 指向 width * height * 2 字节的只读数据。
 */
Status_t LCD_ShowBitmap(uint16_t x, uint16_t y, uint16_t width,
                        uint16_t height, const uint8_t *rgb565);

#endif
