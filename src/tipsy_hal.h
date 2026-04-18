#ifndef TIPSY_HAL_H
#define TIPSY_HAL_H

#include <stdbool.h>
#include <stdint.h>

#define TIPSY_LCD_WIDTH 320
#define TIPSY_LCD_HEIGHT 480

#define TIPSY_COLOR_BLACK 0x0000
#define TIPSY_COLOR_DARKGRAY 0x39E7
#define TIPSY_COLOR_LIGHTGRAY 0xBDF7
#define TIPSY_COLOR_RED 0xF800
#define TIPSY_COLOR_GREEN 0x07E0
#define TIPSY_COLOR_BLUE 0x001F
#define TIPSY_COLOR_WHITE 0xFFFF

typedef struct {
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
} TipsyRect;

typedef struct {
  bool pressed;
  uint16_t x;
  uint16_t y;
} TipsyTouchSample;

void tipsy_hal_init_stdio_and_led(void);
void tipsy_display_init(void);
void tipsy_touch_init(void);
bool tipsy_touch_read(TipsyTouchSample *sample);

void tipsy_display_fill_screen(uint16_t color);
void tipsy_display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                             uint16_t color);
void tipsy_display_draw_text_5x7(uint16_t x, uint16_t y, const char *text,
                                 uint16_t color, uint8_t scale);
void tipsy_display_present(void);
bool tipsy_rect_contains(const TipsyRect *rect, uint16_t x, uint16_t y);

#endif
