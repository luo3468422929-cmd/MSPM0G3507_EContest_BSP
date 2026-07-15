#ifndef LCD_FONT_H
#define LCD_FONT_H

#include "common.h"

/**
 * 按 UTF-8 字符串显示内置的 16×16 常用中文字模。
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
