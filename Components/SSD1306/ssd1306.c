#include "ssd1306.h"

#include <string.h>

typedef struct { char character; uint8_t columns[5]; } Glyph_t;

/* 电赛状态页常用的 5x7 ASCII 字形；小写自动转大写。 */
static const Glyph_t g_font[] = {
    {' ',{0,0,0,0,0}}, {'-',{8,8,8,8,8}}, {'.',{0,96,96,0,0}},
    {':',{0,54,54,0,0}}, {'/',{32,16,8,4,2}}, {'%',{35,19,8,100,98}},
    {'0',{62,81,73,69,62}}, {'1',{0,66,127,64,0}}, {'2',{66,97,81,73,70}},
    {'3',{33,65,69,75,49}}, {'4',{24,20,18,127,16}}, {'5',{39,69,69,69,57}},
    {'6',{60,74,73,73,48}}, {'7',{1,113,9,5,3}}, {'8',{54,73,73,73,54}},
    {'9',{6,73,73,41,30}}, {'A',{126,17,17,17,126}}, {'B',{127,73,73,73,54}},
    {'C',{62,65,65,65,34}}, {'D',{127,65,65,34,28}}, {'E',{127,73,73,73,65}},
    {'F',{127,9,9,9,1}}, {'G',{62,65,73,73,122}}, {'H',{127,8,8,8,127}},
    {'I',{0,65,127,65,0}}, {'J',{32,64,65,63,1}}, {'K',{127,8,20,34,65}},
    {'L',{127,64,64,64,64}}, {'M',{127,2,12,2,127}}, {'N',{127,4,8,16,127}},
    {'O',{62,65,65,65,62}}, {'P',{127,9,9,9,6}}, {'Q',{62,65,81,33,94}},
    {'R',{127,9,25,41,70}}, {'S',{70,73,73,73,49}}, {'T',{1,1,127,1,1}},
    {'U',{63,64,64,64,63}}, {'V',{31,32,64,32,31}}, {'W',{63,64,56,64,63}},
    {'X',{99,20,8,20,99}}, {'Y',{3,4,120,4,3}}, {'Z',{97,81,73,69,67}},
    {'?',{2,1,81,9,6}}
};

static const uint8_t *SSD1306_FindGlyph(char character)
{
    if ((character >= 'a') && (character <= 'z')) {
        character = (char)(character - 'a' + 'A');
    }
    for (uint16_t index = 0U; index < ARRAY_SIZE(g_font); ++index) {
        if (g_font[index].character == character) { return g_font[index].columns; }
    }
    return g_font[ARRAY_SIZE(g_font) - 1U].columns;
}

Status_t SSD1306_Init(SSD1306_t *display, SSD1306_WriteFn_t writeFunction)
{
    static const uint8_t initSequence[] = {
        0xAE,0x20,0x00,0xB0,0xC8,0x00,0x10,0x40,0x81,0x7F,
        0xA1,0xA6,0xA8,0x3F,0xA4,0xD3,0x00,0xD5,0x80,0xD9,
        0xF1,0xDA,0x12,0xDB,0x40,0x8D,0x14,0xAF
    };
    if ((display == NULL) || (writeFunction == NULL)) { return STATUS_INVALID_PARAM; }
    memset(display, 0, sizeof(*display));
    display->write = writeFunction;
    if (display->write(false, initSequence, sizeof(initSequence)) != STATUS_OK) {
        return STATUS_ERROR;
    }
    display->initialized = true;
    return SSD1306_Refresh(display);
}

void SSD1306_Clear(SSD1306_t *display)
{
    if (display != NULL) { memset(display->buffer, 0, sizeof(display->buffer)); }
}

void SSD1306_DrawPixel(SSD1306_t *display, uint8_t x, uint8_t y, bool on)
{
    uint16_t index;
    uint8_t mask;
    if ((display == NULL) || (x >= SSD1306_WIDTH) || (y >= SSD1306_HEIGHT)) { return; }
    index = (uint16_t)x + ((uint16_t)(y / 8U) * SSD1306_WIDTH);
    mask = (uint8_t)(1U << (y & 7U));
    if (on) { display->buffer[index] |= mask; }
    else { display->buffer[index] &= (uint8_t)~mask; }
}

void SSD1306_DrawChar(SSD1306_t *display, uint8_t x, uint8_t y, char character)
{
    const uint8_t *glyph = SSD1306_FindGlyph(character);
    for (uint8_t column = 0U; column < 5U; ++column) {
        for (uint8_t row = 0U; row < 7U; ++row) {
            SSD1306_DrawPixel(display, (uint8_t)(x + column), (uint8_t)(y + row),
                              ((glyph[column] >> row) & 1U) != 0U);
        }
    }
}

void SSD1306_DrawString(SSD1306_t *display, uint8_t x, uint8_t y, const char *text)
{
    if ((display == NULL) || (text == NULL)) { return; }
    while ((*text != '\0') && (x <= (SSD1306_WIDTH - 6U))) {
        SSD1306_DrawChar(display, x, y, *text++);
        x = (uint8_t)(x + 6U);
    }
}

Status_t SSD1306_Refresh(SSD1306_t *display)
{
    uint8_t commands[3];
    if ((display == NULL) || !display->initialized) { return STATUS_NOT_INITIALIZED; }
    for (uint8_t page = 0U; page < 8U; ++page) {
        commands[0] = (uint8_t)(0xB0U + page);
        commands[1] = 0x00U;
        commands[2] = 0x10U;
        if (display->write(false, commands, sizeof(commands)) != STATUS_OK) {
            return STATUS_ERROR;
        }
        if (display->write(true, &display->buffer[(uint16_t)page * SSD1306_WIDTH],
                           SSD1306_WIDTH) != STATUS_OK) {
            return STATUS_ERROR;
        }
    }
    return STATUS_OK;
}

