/**
 * @file lcd.h
 * @brief 定义 128×160 ST7735S 的初始化、RGB565 绘图和 ASCII 数值显示接口。
 *
 * 所属层：Hardware 显示器件层。底层使用 Bsp/spi 阻塞发送，无全屏帧缓冲；
 * 高频控制任务应分行更新。LCD BL 已直连 3.3 V，不使用 PA24。
 */
#ifndef LCD_H
#define LCD_H

#include "common.h"

/** 当前屏幕逻辑方向下的可绘制尺寸。 */
#define LCD_WIDTH                         128U
#define LCD_HEIGHT                        160U

/** 常用 RGB565 颜色；可直接传给 Fill/Show 系列接口。 */
#define LCD_COLOR_BLACK                   0x0000U
#define LCD_COLOR_WHITE                   0xFFFFU
#define LCD_COLOR_RED                     0xF800U
#define LCD_COLOR_GREEN                   0x07E0U
#define LCD_COLOR_BLUE                    0x001FU
#define LCD_COLOR_YELLOW                  0xFFE0U
#define LCD_COLOR_CYAN                    0x07FFU

/** @brief 硬复位并初始化 ST7735S，最后清为黑屏；BL 硬件常亮。 */
Status_t LCD_Init(void);
/** @brief 全屏填充一种 RGB565 颜色。 */
Status_t LCD_Clear(uint16_t color);
/** @brief 填充闭区间矩形；坐标越界或顺序错误返回 STATUS_INVALID_PARAM。 */
Status_t LCD_Fill(uint16_t x1, uint16_t y1,
                  uint16_t x2, uint16_t y2, uint16_t color);
/**
 * @brief 向连续矩形窗口写入 RGB565 原始数据，供字体/图片扩展模块使用。
 * @param x,y 目标区域左上角。
 * @param width,height 目标区域尺寸，必须完全位于屏幕内。
 * @param data 行优先像素字节，高字节在前。
 * length 必须等于 width * height * 2，数据高字节在前。
 */
Status_t LCD_WriteRegion(uint16_t x, uint16_t y,
                         uint16_t width, uint16_t height,
                         const uint8_t *data, uint32_t length);
/** @brief 显示 5×7 ASCII 字符串；每字符实际占 6×8，小写按大写显示。 */
Status_t LCD_ShowString(uint16_t x, uint16_t y, const char *text,
                        uint16_t color, uint16_t background);

/** @brief 将 int32 格式化为十进制字符串后显示。 */
Status_t LCD_ShowInt(uint16_t x, uint16_t y, int32_t value,
                     uint16_t color, uint16_t background);

/** @brief 显示浮点数，decimals 合法范围 0~6；依赖 TI libc 浮点格式化。 */
Status_t LCD_ShowFloat(uint16_t x, uint16_t y, float value,
                       uint8_t decimals, uint16_t color,
                       uint16_t background);
/** @brief 兼容旧调用；当前 BL 直连 3.3 V，因此本函数不会操作 GPIO。 */
void LCD_SetBacklight(bool on);

#endif
