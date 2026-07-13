#ifndef SSD1306_H
#define SSD1306_H

#include "common.h"

#define SSD1306_WIDTH  128U
#define SSD1306_HEIGHT 64U
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8U)

typedef Status_t (*SSD1306_WriteFn_t)(bool dataMode,
                                     const uint8_t *data,
                                     uint16_t length);

typedef struct {
    uint8_t buffer[SSD1306_BUFFER_SIZE];
    SSD1306_WriteFn_t write;
    bool initialized;
} SSD1306_t;

Status_t SSD1306_Init(SSD1306_t *display, SSD1306_WriteFn_t writeFunction);
void SSD1306_Clear(SSD1306_t *display);
void SSD1306_DrawPixel(SSD1306_t *display, uint8_t x, uint8_t y, bool on);
void SSD1306_DrawChar(SSD1306_t *display, uint8_t x, uint8_t y, char character);
void SSD1306_DrawString(SSD1306_t *display, uint8_t x, uint8_t y, const char *text);
Status_t SSD1306_Refresh(SSD1306_t *display);

#endif

