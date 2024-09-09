#include <string.h>
#include "twi.h"
#include "ssd1306.h"

#define SSD1306_ADDR    0x3C

#define FONT_WIDTH  5
#define FONT_HEIGHT 8

static const uint8_t FONT[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // ' '
    0x00, 0x00, 0x5F, 0x00, 0x00, // '!'
    0x00, 0x07, 0x00, 0x07, 0x00, // '"'
    0x14, 0x3E, 0x14, 0x3E, 0x14, // '#'
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // '$'
    0x23, 0x13, 0x08, 0x64, 0x62, // '%'
    0x36, 0x49, 0x55, 0x22, 0x50, // '&'
    0x00, 0x04, 0x02, 0x01, 0x00, // '\''
    0x00, 0x1C, 0x22, 0x41, 0x00, // '('
    0x00, 0x41, 0x22, 0x1C, 0x00, // ')'
    0x14, 0x08, 0x3E, 0x08, 0x14, // '*'
    0x08, 0x08, 0x3E, 0x08, 0x08, // '+'
    0x00, 0xA0, 0x60, 0x00, 0x00, // ','
    0x08, 0x08, 0x08, 0x08, 0x08, // '-'
    0x00, 0x60, 0x60, 0x00, 0x00, // '.'
    0x20, 0x10, 0x08, 0x04, 0x02, // '/'
    0x3E, 0x51, 0x49, 0x45, 0x3E, // '0'
    0x04, 0x42, 0x7F, 0x40, 0x00, // '1'
    0x62, 0x51, 0x49, 0x49, 0x46, // '2'
    0x22, 0x41, 0x49, 0x49, 0x36, // '3'
    0x18, 0x14, 0x12, 0x7F, 0x10, // '4'
    0x27, 0x45, 0x45, 0x45, 0x39, // '5'
    0x3C, 0x4A, 0x49, 0x49, 0x30, // '6'
    0x01, 0x71, 0x09, 0x05, 0x03, // '7'
    0x36, 0x49, 0x49, 0x49, 0x36, // '8'
    0x06, 0x49, 0x49, 0x29, 0x1E, // '9'
    0x00, 0x36, 0x36, 0x00, 0x00, // ':'
    0x00, 0x56, 0x36, 0x00, 0x00, // ';'
    0x08, 0x14, 0x22, 0x41, 0x00, // '<'
    0x14, 0x14, 0x14, 0x14, 0x14, // '='
    0x41, 0x22, 0x14, 0x08, 0x00, // '>'
    0x02, 0x01, 0x51, 0x09, 0x06, // '?'
    0x32, 0x49, 0x79, 0x41, 0x3E, // '@'
    0x7C, 0x12, 0x11, 0x12, 0x7C, // 'A'
    0x41, 0x7F, 0x49, 0x49, 0x36, // 'B'
    0x3E, 0x41, 0x41, 0x41, 0x22, // 'C'
    0x41, 0x7F, 0x41, 0x41, 0x3E, // 'D'
    0x7F, 0x49, 0x49, 0x49, 0x41, // 'E'
    0x7F, 0x09, 0x09, 0x09, 0x01, // 'F'
    0x3E, 0x41, 0x41, 0x51, 0x72, // 'G'
    0x7F, 0x08, 0x08, 0x08, 0x7F, // 'H'
    0x00, 0x41, 0x7F, 0x41, 0x00, // 'I'
    0x20, 0x40, 0x41, 0x3F, 0x01, // 'J'
    0x7F, 0x08, 0x14, 0x22, 0x41, // 'K'
    0x7F, 0x40, 0x40, 0x40, 0x40, // 'L'
    0x7F, 0x02, 0x0C, 0x02, 0x7F, // 'M'
    0x7F, 0x04, 0x08, 0x10, 0x7F, // 'N'
    0x3E, 0x41, 0x41, 0x41, 0x3E, // 'O'
    0x7F, 0x09, 0x09, 0x09, 0x06, // 'P'
    0x3E, 0x41, 0x51, 0x21, 0x5E, // 'Q'
    0x7F, 0x09, 0x19, 0x29, 0x46, // 'R'
    0x26, 0x49, 0x49, 0x49, 0x32, // 'S'
    0x01, 0x01, 0x7F, 0x01, 0x01, // 'T'
    0x3F, 0x40, 0x40, 0x40, 0x3F, // 'U'
    0x1F, 0x20, 0x40, 0x20, 0x1F, // 'V'
    0x3F, 0x40, 0x38, 0x40, 0x3F, // 'W'
    0x63, 0x14, 0x08, 0x14, 0x63, // 'X'
    0x07, 0x08, 0x70, 0x08, 0x07, // 'Y'
    0x61, 0x51, 0x49, 0x45, 0x43, // 'Z'
    0x00, 0x7F, 0x41, 0x41, 0x00, // '['
    0x02, 0x04, 0x08, 0x10, 0x20, // '\\'
    0x00, 0x41, 0x41, 0x7F, 0x00, // ']'
    0x04, 0x02, 0x01, 0x02, 0x04, // '^'
    0x80, 0x80, 0x80, 0x80, 0x80, // '_'
    0x00, 0x01, 0x02, 0x04, 0x00, // '`'
    0x20, 0x54, 0x54, 0x54, 0x78, // 'a'
    0x7F, 0x48, 0x44, 0x44, 0x38, // 'b'
    0x38, 0x44, 0x44, 0x44, 0x20, // 'c'
    0x38, 0x44, 0x44, 0x48, 0x7F, // 'd'
    0x38, 0x54, 0x54, 0x54, 0x18, // 'e'
    0x08, 0x7E, 0x09, 0x01, 0x02, // 'f'
    0x18, 0xA4, 0xA4, 0xA4, 0x7C, // 'g'
    0x7F, 0x08, 0x04, 0x04, 0x78, // 'h'
    0x00, 0x48, 0x7D, 0x40, 0x00, // 'i'
    0x20, 0x40, 0x44, 0x3D, 0x00, // 'j'
    0x7F, 0x10, 0x28, 0x44, 0x00, // 'k'
    0x00, 0x41, 0x7F, 0x40, 0x00, // 'l'
    0x7C, 0x04, 0x78, 0x04, 0x78, // 'm'
    0x7C, 0x08, 0x04, 0x04, 0x78, // 'n'
    0x38, 0x44, 0x44, 0x44, 0x38, // 'o'
    0xFC, 0x24, 0x24, 0x24, 0x18, // 'p'
    0x18, 0x24, 0x24, 0x28, 0xFC, // 'q'
    0x7C, 0x08, 0x04, 0x04, 0x08, // 'r'
    0x48, 0x54, 0x54, 0x54, 0x20, // 's'
    0x04, 0x3F, 0x44, 0x40, 0x20, // 't'
    0x3C, 0x40, 0x40, 0x20, 0x7C, // 'u'
    0x1C, 0x20, 0x40, 0x20, 0x1C, // 'v'
    0x3C, 0x40, 0x30, 0x40, 0x3C, // 'w'
    0x44, 0x28, 0x10, 0x28, 0x44, // 'x'
    0x1C, 0xA0, 0xA0, 0x90, 0x7C, // 'y'
    0x44, 0x64, 0x54, 0x4C, 0x44, // 'z'
    0x00, 0x08, 0x36, 0x41, 0x00, // '{'
    0x00, 0x00, 0x7F, 0x00, 0x00, // '|'
    0x00, 0x41, 0x36, 0x08, 0x00, // '}'
    0x08, 0x04, 0x04, 0x08, 0x04, // '~'
};

static uint8_t _screen[OLED_WIDTH * (OLED_HEIGHT / 8) + 1];

bool oled_begin() {
    const uint8_t CMDS[] = {
        0x00,
        0xAE, // DISPLAYOFF
        0xD5, 0x80, // SETDISPLAYCLOCKDIV = 0x80
        0xA8, 0x3F, // SETMULTIPLEX
        0xD3, 0x00, // SETDISPLAYOFFSET = 0x00
        0x40 | 0, // SETSTARTLINE
        0x8D, 0x14, // CHARGEPUMP = 0x14
        0xA0 | 0x01, // SEGREMAP = reverse
        0xC8, // COMSCANDEC
        0xDA, 0x12, // SETCOMPINS
        0xD9, 0x22, // SETPRECHARGE = 0x22
        0xDB, 0x40, // SETVCOMDETECT
        0x20, 0x00, // MEMORYMODE = HORIZONTAL_ADDRESSING_MODE
        0x21, 0x00, 0x7F, // SETCOLUMN
        0x22, 0x00, 0x07, // SETPAGE
        0x81, 0xCF, // SETCONTRAST
        0xA4, // DISPLAYALLON_RESUME
        0xA6, // NORMALDISPLAY
        0xAF // DISPLAYON
    };

    if (TWI_Start(SSD1306_ADDR, false) == TWI_OK) {
        TWI_Write(CMDS, sizeof(CMDS), TWI_FLAG_AUTOSTOP | TWI_FLAG_WAIT);
        _screen[0] = 0x40;
        oled_clear();
        return true;
    }
    return false;
}

bool oled_flip(bool on) {
    if (TWI_Start(SSD1306_ADDR, false) == TWI_OK) {
        uint8_t CMDS[3];

        CMDS[0] = 0x00;
        CMDS[1] = on ? 0xA0 : 0xA0 | 0x01; // SEGREMAP = normal/reverse
        CMDS[2] = on ? 0xC0 : 0xC8; // COMSCANINC/COMSCANDEC
        TWI_Write(CMDS, sizeof(CMDS), TWI_FLAG_AUTOSTOP | TWI_FLAG_WAIT);
        return true;
    }
    return false;
}

bool oled_invert(bool on) {
    if (TWI_Start(SSD1306_ADDR, false) == TWI_OK) {
        uint8_t CMDS[2];

        CMDS[0] = 0x00;
        CMDS[1] = 0xA6 | on; // NORMALDISPLAY/INVERSEDISPLAY
        TWI_Write(CMDS, sizeof(CMDS), TWI_FLAG_AUTOSTOP | TWI_FLAG_WAIT);
        return true;
    }
    return false;
}

bool oled_power(bool on) {
    if (TWI_Start(SSD1306_ADDR, false) == TWI_OK) {
        uint8_t CMDS[2];

        CMDS[0] = 0x00;
        CMDS[1] = 0xAE | on; // DISPLAYON/DISPLAYOFF
        TWI_Write(CMDS, sizeof(CMDS), TWI_FLAG_AUTOSTOP | TWI_FLAG_WAIT);
        return true;
    }
    return false;
}

bool oled_contrast(uint8_t value) {
    if (TWI_Start(SSD1306_ADDR, false) == TWI_OK) {
        uint8_t CMDS[3];

        CMDS[0] = 0x00;
        CMDS[1] = 0x81; // SETCONTRAST
        CMDS[2] = value;
        TWI_Write(CMDS, sizeof(CMDS), TWI_FLAG_AUTOSTOP | TWI_FLAG_WAIT);
        return true;
    }
    return false;
}

bool oled_flush(bool wait) {
    if (TWI_Start(SSD1306_ADDR, false) == TWI_OK) {
        TWI_Write(_screen, sizeof(_screen), TWI_FLAG_AUTOSTOP | (wait ? TWI_FLAG_WAIT : 0));
        return true;
    }
    return false;
}

void oled_clear() {
    memset(&_screen[1], 0, sizeof(_screen) - 1);
}

void oled_pixel(uint8_t x, uint8_t y, bool color) {
    uint16_t offset = OLED_WIDTH * (y >> 3) + x + 1;

    if (color)
        _screen[offset] |= (1 << (y & 0x07));
    else
        _screen[offset] &= ~(1 << (y & 0x07));
}

#ifdef FASTBIT
static void update_byte(uint8_t *data, uint8_t and_mask, uint8_t or_mask) {
    *data &= and_mask;
    *data |= or_mask;
}

void oled_drawpattern(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pattern) {
    uint8_t offset, mask, data;

    offset = y & 7;
    if (offset) { // First row is incomplete
        mask = 0xFF;
        if (h < 8)
            mask >>= (8 - h);
        mask <<= offset;
        data = pattern << offset;
        for (uint8_t _x = x; (_x < x + w) && (_x < OLED_WIDTH); ++_x) {
            update_byte(&_screen[OLED_WIDTH * (y >> 3) + _x + 1], ~mask, data & mask);
        }
        if (h <= 8 - offset) // Last row
            return;
        h -= (8 - offset);
        y += (8 - offset);
    }
    while (y < OLED_HEIGHT) {
        mask = 0xFF;
        if (h < 8)
            mask >>= (8 - h);
        if (offset)
            data = (pattern >> (8 - offset)) | (pattern << offset);
        else
            data = pattern;
        for (uint8_t _x = x; (_x < x + w) && (_x < OLED_WIDTH); ++_x) {
            update_byte(&_screen[OLED_WIDTH * (y >> 3) + _x + 1], ~mask, data & mask);
        }
        if (h <= 8) // Last row
            break;
        h -= 8;
        y += 8;
    }
}

void oled_drawpatterns(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *pattern, bool inverse) {
    uint8_t offset, mask, data;

    offset = y & 7;
    if (offset) { // First row is incomplete
        mask = 0xFF;
        if (h < 8)
            mask >>= (8 - h);
        mask <<= offset;
        for (uint8_t _x = x; (_x < x + w) && (_x < OLED_WIDTH); ++_x) {
            data = pattern[_x - x] << offset;
            if (inverse)
                data = ~data;
            update_byte(&_screen[OLED_WIDTH * (y >> 3) + _x + 1], ~mask, data & mask);
        }
        if (h <= 8 - offset) // Last row
            return;
        h -= (8 - offset);
        y += (8 - offset);
    }
    while (y < OLED_HEIGHT) {
        mask = 0xFF;
        if (h < 8)
            mask >>= (8 - h);
        for (uint8_t _x = x; (_x < x + w) && (_x < OLED_WIDTH); ++_x) {
            if (offset)
                data = (pattern[_x - x] >> (8 - offset)) | (pattern[w + _x - x] << offset);
            else
                data = pattern[_x - x];
            if (inverse)
                data = ~data;
            update_byte(&_screen[OLED_WIDTH * (y >> 3) + _x + 1], ~mask, data & mask);
        }
        if (h <= 8) // Last row
            break;
        h -= 8;
        y += 8;
        pattern += w;
    }
}

#else
void oled_drawpattern(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pattern) {
    for (uint8_t _y = y; (_y < y + h) && (_y < OLED_HEIGHT); ++_y) {
        for (uint8_t _x = x; (_x < x + w) && (_x < OLED_WIDTH); ++_x) {
            oled_pixel(_x, _y, (pattern >> ((_y - y) & 0x07)) & 0x01);
        }
    }
}

void oled_drawpatterns(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *pattern, bool inverse) {
    for (uint8_t _y = y; (_y < y + h) && (_y < OLED_HEIGHT); ++_y) {
        for (uint8_t _x = x; (_x < x + w) && (_x < OLED_WIDTH); ++_x) {
            oled_pixel(_x, _y, ((pattern[w * ((_y - y) >> 3) + (_x - x)] >> ((_y - y) & 0x07)) & 0x01) ^ inverse);
        }
    }
}
#endif

/*
void oled_hline(uint8_t x, uint8_t y, uint8_t w, bool color) {
    for (uint8_t _x = x; (_x < x + w) && (_x < OLED_WIDTH); ++_x) {
        oled_pixel(_x, y, color);
    }
}

void oled_vline(uint8_t x, uint8_t y, uint8_t h, bool color) {
    for (uint8_t _y = y; (_y < y + h) && (_y < OLED_HEIGHT); ++_y) {
        oled_pixel(x, _y, color);
    }
}
*/

static inline uint8_t abs8(int8_t value) {
    return value < 0 ? -value : value;
}

static inline int8_t sign8(int8_t value) {
    return value < 0 ? -1 : value > 0 ? 1 : 0;
}

void oled_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool color) {
    if (x1 == x2)
        oled_vline(x1, y1, abs8(y2 - y1) + 1, color);
    else if (y1 == y2)
        oled_hline(x1, y1, abs8(x2 - x1) + 1, color);
    else {
        bool steep;
        uint8_t dX, dY;
        int8_t err, yStep;

        steep = abs8(y2 - y1) > abs8(x2 - x1);
        if (steep) {
            uint8_t t;

            t = x1;
            x1 = y1;
            y1 = t;
            t = x2;
            x2 = y2;
            y2 = t;
        }
        if (x2 < x1) {
            uint8_t t;

            t = x1;
            x1 = x2;
            x2 = t;
            t = y1;
            y1 = y2;
            y2 = t;
        }
        dX = x2 - x1;
        dY = abs8(y2 - y1);
        err = dX / 2;
        yStep = sign8(y2 - y1);
        while (x1 <= x2) {
            if (steep) {
                oled_pixel(y1, x1, color);
            } else {
                oled_pixel(x1, y1, color);
            }
            err -= dY;
            if (err < 0) {
                y1 += yStep;
                err += dX;
            }
            ++x1;
        }
    }
}

void oled_printchar(char c, uint8_t x, uint8_t y, bool color) {
    if (c < ' ')
        c = ' ';
    oled_drawpatterns(x, y, FONT_WIDTH, FONT_HEIGHT, &FONT[(c - ' ') * FONT_WIDTH], ! color);
}

void oled_printchar_x2(char c, uint8_t x, uint8_t y, bool color) {
    uint8_t glyph[FONT_WIDTH * 2 * ((FONT_HEIGHT * 2 + 7) / 8)];

    if (c < ' ')
        c = ' ';
    for (uint8_t i = 0; i < FONT_WIDTH; ++i) {
        uint8_t src = FONT[(c - ' ') * FONT_WIDTH + i];
        uint16_t dest = 0;

        for (int8_t j = 0; j < FONT_HEIGHT; ++j) {
            if ((src >> j) & 0x01)
                dest |= (0x03 << (j * 2));
        }
        glyph[i * 2] = (uint8_t)dest;
        glyph[i * 2 + 1] = (uint8_t)dest;
        glyph[(i + FONT_WIDTH) * 2] = (uint8_t)(dest >> 8);
        glyph[(i + FONT_WIDTH) * 2 + 1] = (uint8_t)(dest >> 8);
    }
    oled_drawpatterns(x, y, FONT_WIDTH * 2, FONT_HEIGHT * 2, glyph, ! color);
}

void oled_printstr(const char *str, uint8_t x, uint8_t y, bool color) {
    while (*str && (x < OLED_WIDTH)) {
        oled_printchar(*str++, x, y, color);
        x += FONT_WIDTH;
        if (*str && (x < OLED_WIDTH))
            oled_drawpattern(x++, y, 1, FONT_HEIGHT, (uint8_t)(color ? 0 : 0xFF));
    }
}

void oled_printstr_x2(const char *str, uint8_t x, uint8_t y, bool color) {
    while (*str && (x < OLED_WIDTH)) {
        oled_printchar_x2(*str++, x, y, color);
        x += FONT_WIDTH * 2;
        if (*str && (x < OLED_WIDTH))
            oled_drawpattern(x++, y, 1, FONT_HEIGHT * 2, (uint8_t)(color ? 0 : 0xFF));
    }
}

uint8_t oled_strwidth(const char *str, bool x2) {
    uint8_t result = 0;

    while (*str++) {
        if (result)
            ++result; // Gap
        result += (FONT_WIDTH << x2);
    }
    return result;
}

inline __attribute__((always_inline)) uint8_t oled_charheight(bool x2) {
    return FONT_HEIGHT << x2;
}
