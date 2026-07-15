#ifndef LCD_H
#define LCD_H

#include "common.h"

#define LCD_WIDTH                         128U
#define LCD_HEIGHT                        160U

#define LCD_COLOR_BLACK                   0x0000U
#define LCD_COLOR_WHITE                   0xFFFFU
#define LCD_COLOR_RED                     0xF800U
#define LCD_COLOR_GREEN                   0x07E0U
#define LCD_COLOR_BLUE                    0x001FU
#define LCD_COLOR_YELLOW                  0xFFE0U
#define LCD_COLOR_CYAN                    0x07FFU

/** 初始化 ST7735S、清屏并开启背光。 */
Status_t LCD_Init(void);
/** 全屏填充 RGB565 颜色。 */
Status_t LCD_Clear(uint16_t color);
/** 填充闭区间矩形，坐标越界返回 STATUS_INVALID_PARAM。 */
Status_t LCD_Fill(uint16_t x1, uint16_t y1,
                  uint16_t x2, uint16_t y2, uint16_t color);
/**
 * 向连续矩形窗口写入 RGB565 原始数据，供字体/图片扩展模块使用。
 * length 必须等于 width * height * 2，数据高字节在前。
 */
Status_t LCD_WriteRegion(uint16_t x, uint16_t y,
                         uint16_t width, uint16_t height,
                         const uint8_t *data, uint32_t length);
/** 显示 5x7 ASCII 字符串，小写按大写显示。 */
Status_t LCD_ShowString(uint16_t x, uint16_t y, const char *text,
                        uint16_t color, uint16_t background);
Status_t LCD_ShowInt(uint16_t x, uint16_t y, int32_t value,
                     uint16_t color, uint16_t background);
Status_t LCD_ShowFloat(uint16_t x, uint16_t y, float value,
                       uint8_t decimals, uint16_t color,
                       uint16_t background);
void LCD_SetBacklight(bool on);

#endif
