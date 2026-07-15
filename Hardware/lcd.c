#include "lcd.h"

#include <stdio.h>

#include "board_pins.h"
#include "spi.h"
#include "timer.h"
#include "user_config.h"

typedef struct {
    char character;
    uint8_t columns[5];
} LCD_Glyph_t;

/* 电赛状态页常用 5x7 ASCII；比旧 OLED 字库补齐 ! ( ) + =。 */
static const LCD_Glyph_t g_font[] = {
    {' ',{0,0,0,0,0}}, {'!',{0,0,95,0,0}}, {'(',{0,28,34,65,0}},
    {')',{0,65,34,28,0}}, {'+',{8,8,62,8,8}}, {'-',{8,8,8,8,8}},
    {'.',{0,96,96,0,0}}, {'/',{32,16,8,4,2}}, {':',{0,54,54,0,0}},
    {'=',{20,20,20,20,20}}, {'%',{35,19,8,100,98}},
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

static bool g_initialized;

static const uint8_t *LCD_FindGlyph(char character)
{
    if ((character >= 'a') && (character <= 'z')) {
        character = (char)(character - 'a' + 'A');
    }
    for (uint16_t index = 0U; index < ARRAY_SIZE(g_font); ++index) {
        if (g_font[index].character == character) {
            return g_font[index].columns;
        }
    }
    return g_font[ARRAY_SIZE(g_font) - 1U].columns;
}

static Status_t LCD_WriteCommandData(uint8_t command,
                                     const uint8_t *data,
                                     uint8_t length)
{
    Status_t status;
    if ((data == NULL) && (length != 0U)) {
        return STATUS_INVALID_PARAM;
    }

    DL_GPIO_clearPins(PIN_LCD_CS_PORT, PIN_LCD_CS);
    DL_GPIO_clearPins(PIN_LCD_DC_PORT, PIN_LCD_DC);
    status = SPI_Write(&command, 1U);
    if ((status == STATUS_OK) && (length != 0U)) {
        DL_GPIO_setPins(PIN_LCD_DC_PORT, PIN_LCD_DC);
        status = SPI_Write(data, length);
    }
    DL_GPIO_setPins(PIN_LCD_CS_PORT, PIN_LCD_CS);
    return status;
}

static Status_t LCD_SetAddressWindow(uint16_t x1, uint16_t y1,
                                     uint16_t x2, uint16_t y2)
{
    uint16_t startX = (uint16_t)(x1 + LCD_X_OFFSET);
    uint16_t endX = (uint16_t)(x2 + LCD_X_OFFSET);
    uint16_t startY = (uint16_t)(y1 + LCD_Y_OFFSET);
    uint16_t endY = (uint16_t)(y2 + LCD_Y_OFFSET);
    uint8_t data[4];
    Status_t status;

    data[0] = (uint8_t)(startX >> 8U);
    data[1] = (uint8_t)startX;
    data[2] = (uint8_t)(endX >> 8U);
    data[3] = (uint8_t)endX;
    status = LCD_WriteCommandData(0x2AU, data, sizeof(data));
    if (status != STATUS_OK) { return status; }

    data[0] = (uint8_t)(startY >> 8U);
    data[1] = (uint8_t)startY;
    data[2] = (uint8_t)(endY >> 8U);
    data[3] = (uint8_t)endY;
    status = LCD_WriteCommandData(0x2BU, data, sizeof(data));
    if (status != STATUS_OK) { return status; }
    return LCD_WriteCommandData(0x2CU, NULL, 0U);
}

static Status_t LCD_WritePixels(const uint8_t *data, uint16_t length)
{
    Status_t status;
    DL_GPIO_clearPins(PIN_LCD_CS_PORT, PIN_LCD_CS);
    DL_GPIO_setPins(PIN_LCD_DC_PORT, PIN_LCD_DC);
    status = SPI_Write(data, length);
    DL_GPIO_setPins(PIN_LCD_CS_PORT, PIN_LCD_CS);
    return status;
}

static Status_t LCD_SendInit(uint8_t command,
                             const uint8_t *data,
                             uint8_t length)
{
    return LCD_WriteCommandData(command, data, length);
}

Status_t LCD_Init(void)
{
    static const uint8_t frameRate[] = {0x05U, 0x3CU, 0x3CU};
    static const uint8_t frameRate3[] = {0x05U, 0x3CU, 0x3CU, 0x05U, 0x3CU, 0x3CU};
    static const uint8_t power0[] = {0x28U, 0x08U, 0x04U};
    static const uint8_t power2[] = {0x0DU, 0x00U};
    static const uint8_t power3[] = {0x8DU, 0x2AU};
    static const uint8_t power4[] = {0x8DU, 0xEEU};
    static const uint8_t gammaPositive[] = {
        0x04U,0x22U,0x07U,0x0AU,0x2EU,0x30U,0x25U,0x2AU,
        0x28U,0x26U,0x2EU,0x3AU,0x00U,0x01U,0x03U,0x13U
    };
    static const uint8_t gammaNegative[] = {
        0x04U,0x16U,0x06U,0x0DU,0x2DU,0x26U,0x23U,0x27U,
        0x27U,0x25U,0x2DU,0x3BU,0x00U,0x01U,0x04U,0x13U
    };
    const uint8_t inversion = 0x03U;
    const uint8_t power1 = 0xC0U;
    const uint8_t vcom = 0x1AU;
    const uint8_t madctl = LCD_MADCTL;
    const uint8_t colorMode = 0x05U;
    Status_t status = SPI_Init();
    if (status != STATUS_OK) { return status; }

    g_initialized = false;
    LCD_SetBacklight(false);
    DL_GPIO_clearPins(PIN_LCD_RES_PORT, PIN_LCD_RES);
    Timer_DelayMs(100U);
    DL_GPIO_setPins(PIN_LCD_RES_PORT, PIN_LCD_RES);
    Timer_DelayMs(100U);

#define LCD_INIT(command, data, length) do { \
    status = LCD_SendInit((command), (data), (length)); \
    if (status != STATUS_OK) { return status; } \
} while (0)

    LCD_INIT(0x11U, NULL, 0U);
    Timer_DelayMs(120U);
    LCD_INIT(0xB1U, frameRate, sizeof(frameRate));
    LCD_INIT(0xB2U, frameRate, sizeof(frameRate));
    LCD_INIT(0xB3U, frameRate3, sizeof(frameRate3));
    LCD_INIT(0xB4U, &inversion, 1U);
    LCD_INIT(0xC0U, power0, sizeof(power0));
    LCD_INIT(0xC1U, &power1, 1U);
    LCD_INIT(0xC2U, power2, sizeof(power2));
    LCD_INIT(0xC3U, power3, sizeof(power3));
    LCD_INIT(0xC4U, power4, sizeof(power4));
    LCD_INIT(0xC5U, &vcom, 1U);
    LCD_INIT(0x36U, &madctl, 1U);
    LCD_INIT(0xE0U, gammaPositive, sizeof(gammaPositive));
    LCD_INIT(0xE1U, gammaNegative, sizeof(gammaNegative));
    LCD_INIT(0x3AU, &colorMode, 1U);
    LCD_INIT(0x29U, NULL, 0U);
#undef LCD_INIT

    g_initialized = true;
    status = LCD_Clear(LCD_COLOR_BLACK);
    if (status == STATUS_OK) {
        LCD_SetBacklight(true);
    }
    return status;
}

Status_t LCD_Fill(uint16_t x1, uint16_t y1,
                  uint16_t x2, uint16_t y2, uint16_t color)
{
    uint8_t pixels[64];
    uint32_t remaining;
    Status_t status;

    if (!g_initialized) { return STATUS_NOT_INITIALIZED; }
    if ((x1 > x2) || (y1 > y2) || (x2 >= LCD_WIDTH) || (y2 >= LCD_HEIGHT)) {
        return STATUS_INVALID_PARAM;
    }
    status = LCD_SetAddressWindow(x1, y1, x2, y2);
    if (status != STATUS_OK) { return status; }

    for (uint16_t index = 0U; index < sizeof(pixels); index += 2U) {
        pixels[index] = (uint8_t)(color >> 8U);
        pixels[index + 1U] = (uint8_t)color;
    }
    remaining = ((uint32_t)(x2 - x1 + 1U) * (uint32_t)(y2 - y1 + 1U)) * 2U;
    while (remaining != 0U) {
        uint16_t chunk = (remaining > sizeof(pixels)) ?
                         (uint16_t)sizeof(pixels) : (uint16_t)remaining;
        status = LCD_WritePixels(pixels, chunk);
        if (status != STATUS_OK) { return status; }
        remaining -= chunk;
    }
    return STATUS_OK;
}

Status_t LCD_Clear(uint16_t color)
{
    return LCD_Fill(0U, 0U, LCD_WIDTH - 1U, LCD_HEIGHT - 1U, color);
}

Status_t LCD_WriteRegion(uint16_t x, uint16_t y,
                         uint16_t width, uint16_t height,
                         const uint8_t *data, uint32_t length)
{
    uint32_t expected;
    Status_t status;

    if (!g_initialized) { return STATUS_NOT_INITIALIZED; }
    if ((data == NULL) || (width == 0U) || (height == 0U)) {
        return STATUS_INVALID_PARAM;
    }
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT) ||
        (width > (LCD_WIDTH - x)) || (height > (LCD_HEIGHT - y))) {
        return STATUS_INVALID_PARAM;
    }
    expected = (uint32_t)width * (uint32_t)height * 2U;
    if (length != expected) { return STATUS_INVALID_PARAM; }

    status = LCD_SetAddressWindow(x, y,
                                  (uint16_t)(x + width - 1U),
                                  (uint16_t)(y + height - 1U));
    if (status != STATUS_OK) { return status; }
    while (length != 0U) {
        uint16_t chunk = (length > 4096U) ? 4096U : (uint16_t)length;
        status = LCD_WritePixels(data, chunk);
        if (status != STATUS_OK) { return status; }
        data += chunk;
        length -= chunk;
    }
    return STATUS_OK;
}

static Status_t LCD_ShowChar(uint16_t x, uint16_t y, char character,
                             uint16_t color, uint16_t background)
{
    uint8_t pixels[6U * 8U * 2U];
    const uint8_t *glyph;
    uint16_t output = 0U;
    Status_t status;

    if ((x + 5U >= LCD_WIDTH) || (y + 7U >= LCD_HEIGHT)) {
        return STATUS_INVALID_PARAM;
    }
    glyph = LCD_FindGlyph(character);
    for (uint8_t row = 0U; row < 8U; ++row) {
        for (uint8_t column = 0U; column < 6U; ++column) {
            bool on = (column < 5U) && (((glyph[column] >> row) & 1U) != 0U);
            uint16_t pixel = on ? color : background;
            pixels[output++] = (uint8_t)(pixel >> 8U);
            pixels[output++] = (uint8_t)pixel;
        }
    }
    status = LCD_SetAddressWindow(x, y, (uint16_t)(x + 5U), (uint16_t)(y + 7U));
    return (status == STATUS_OK) ? LCD_WritePixels(pixels, sizeof(pixels)) : status;
}

Status_t LCD_ShowString(uint16_t x, uint16_t y, const char *text,
                        uint16_t color, uint16_t background)
{
    if (!g_initialized) { return STATUS_NOT_INITIALIZED; }
    if (text == NULL) { return STATUS_INVALID_PARAM; }
    while (*text != '\0') {
        Status_t status;
        if (x + 5U >= LCD_WIDTH) {
            return STATUS_OVERFLOW;
        }
        status = LCD_ShowChar(x, y, *text++, color, background);
        if (status != STATUS_OK) { return status; }
        x = (uint16_t)(x + 6U);
    }
    return STATUS_OK;
}

Status_t LCD_ShowInt(uint16_t x, uint16_t y, int32_t value,
                     uint16_t color, uint16_t background)
{
    char text[16];
    int result = snprintf(text, sizeof(text), "%ld", (long)value);
    if ((result < 0) || ((size_t)result >= sizeof(text))) {
        return STATUS_ERROR;
    }
    return LCD_ShowString(x, y, text, color, background);
}

Status_t LCD_ShowFloat(uint16_t x, uint16_t y, float value,
                       uint8_t decimals, uint16_t color,
                       uint16_t background)
{
    char text[24];
    int result;
    if (decimals > 6U) { return STATUS_INVALID_PARAM; }
    result = snprintf(text, sizeof(text), "%.*f", (int)decimals, (double)value);
    if ((result < 0) || ((size_t)result >= sizeof(text))) {
        return STATUS_ERROR;
    }
    return LCD_ShowString(x, y, text, color, background);
}

void LCD_SetBacklight(bool on)
{
    if (on) {
        DL_GPIO_setPins(PIN_LCD_BL_PORT, PIN_LCD_BL);
    } else {
        DL_GPIO_clearPins(PIN_LCD_BL_PORT, PIN_LCD_BL);
    }
}
