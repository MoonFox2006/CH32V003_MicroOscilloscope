#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FASTBIT

#define OLED_WIDTH  128
#define OLED_HEIGHT 64

bool oled_begin();
bool oled_flip(bool on);
bool oled_invert(bool on);
bool oled_power(bool on);
bool oled_contrast(uint8_t value);
bool oled_flush(bool wait);
void oled_clear();
void oled_pixel(uint8_t x, uint8_t y, bool color);
void oled_drawpattern(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pattern);
void oled_drawpatterns(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *pattern, bool inverse);
inline __attribute__((always_inline)) void oled_fillrect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color) {
    oled_drawpattern(x, y, w, h, (uint8_t)(color ? 0xFF : 0x00));
}
inline __attribute__((always_inline)) void oled_hline(uint8_t x, uint8_t y, uint8_t w, bool color) {
    oled_drawpattern(x, y, w, 1, (uint8_t)(color ? 0xFF : 0x00));
}
inline __attribute__((always_inline)) void oled_vline(uint8_t x, uint8_t y, uint8_t h, bool color) {
    oled_drawpattern(x, y, 1, h, (uint8_t)(color ? 0xFF : 0x00));
}
void oled_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool color);
void oled_printchar(char c, uint8_t x, uint8_t y, bool color);
void oled_printchar_x2(char c, uint8_t x, uint8_t y, bool color);
void oled_printstr(const char *str, uint8_t x, uint8_t y, bool color);
void oled_printstr_x2(const char *str, uint8_t x, uint8_t y, bool color);
uint8_t oled_strwidth(const char *str, bool x2);
uint8_t oled_charheight(bool x2);
