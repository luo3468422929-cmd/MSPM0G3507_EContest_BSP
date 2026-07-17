/**
 * @file lcd_font.h
 * @brief 定义内置 16×16 常用中文字模的 UTF-8 显示接口。
 *
 * 所属层：Hardware LCD 扩展。只支持 lcd_font.c 中收录的汉字；普通 ASCII
 * 会转交 LCD_ShowString()。增加汉字时需同时添加 UTF-8 三字节和 32 字节字模。
 */
#ifndef LCD_FONT_H
#define LCD_FONT_H

#include "common.h"

/**
 * @brief 按 UTF-8 字符串显示内置的 16×16 常用中文字模。
 *
 * 入口参数：
 *   x/y        左上角坐标；
 *   text       UTF-8 字符串，只支持工程内置字模；
 *   color      前景色，RGB565；
 *   background 背景色，RGB565。
 * 返回值：参数非法、字符未收录或坐标越界时返回错误码。
 */
Status_t LCD_ShowChinese16(uint16_t x, uint16_t y, const char *text,
                           uint16_t color, uint16_t background);

#endif
