# LCD 中文字体与图片扩展设计

## 目标

在已验证的 ST7735S SPI LCD 驱动上增加中文字符和 RGB565 图片显示能力，同时保持现有英文、数字和颜色接口不变。移植只使用 TI Driverlib 工程中的 SPI、GPIO 和定时器接口，不引入商家 STM32 工程依赖。

## 范围

本次增加两类能力：

1. 16×16 中文点阵显示，支持 UTF-8 中文字符串转换为工程内置的常用字点阵。
2. RGB565 原始图片显示，支持指定位置、宽度、高度和字节数组。

商家完整 12/16/24/32 点阵字库不整体加入固件。首版只加入电赛状态页常用字，后续按需要追加字模，避免一次占用约 79 KB Flash。商家 `pic.h` 中的示例图片不直接作为默认页面显示，仅保留通用图片接口。

## 文件与分层

```text
Hardware/lcd.c/.h          ST7735S 初始化、坐标窗口、SPI 像素写入、ASCII 显示
Hardware/lcd_font.c/.h    中文字模表、单字和中文字符串绘制
Hardware/lcd_bitmap.c/.h  RGB565 图片边界检查和连续像素发送
```

`lcd_font` 和 `lcd_bitmap` 只能调用 `lcd` 提供的受控绘图接口，不直接调用 Driverlib。`User` 层只依赖这些公共接口。

## 公共接口

```c
Status_t LCD_ShowChinese16(uint16_t x, uint16_t y, const char *text,
                           uint16_t color, uint16_t background);
Status_t LCD_ShowBitmap(uint16_t x, uint16_t y, uint16_t width,
                        uint16_t height, const uint8_t *rgb565);
```

中文字符串使用 UTF-8；只匹配内置常用字，未收录字符显示空白方框并返回 `STATUS_INVALID_PARAM`。图片数组按行优先、每像素两字节、高字节在前（与 ST7735S RGB565 写入顺序一致）。

## 资源与边界

- 字模存储在 `const` Flash 区，不使用动态内存。
- 中文每字 16×16 像素，占 32 字节；首版目标为 40 个以内常用字。
- 图片坐标必须满足 `x + width <= LCD_WIDTH`、`y + height <= LCD_HEIGHT`，否则不发送数据并返回 `STATUS_INVALID_PARAM`。
- `NULL` 文本或图片指针返回 `STATUS_INVALID_PARAM`。
- 现有 LCD 偏移和 MADCTL 配置继续由 `User/user_config.h` 控制。

## 测试验收

增加 LCD 扩展静态检查和板级测试页面：

- 编译检查确认 `LCD_ShowChinese16`、`LCD_ShowBitmap` 声明和实现一致。
- 中文测试页显示“电赛循迹”或等价常用字。
- 图片测试页显示一张小尺寸 RGB565 图片。
- 越界、空指针和未收录字符路径返回错误，不影响后续任务调度。
- 原有四色块、`LCD OK` 测试继续通过。

## 不在本次范围内

- 完整 12/24/32 点阵字库。
- PNG/JPEG 解码、文件系统和帧缓冲。
- DMA 图片刷新和整屏双缓冲。
- 修改已经验证通过的 LCD 引脚、SPI 模式和初始化时序。
