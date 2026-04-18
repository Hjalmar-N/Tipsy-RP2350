#include "tipsy_hal.h"

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

#define LCD_RST_PIN 23
#define LCD_DC_PIN 20
#define LCD_BL_PIN 22
#define LCD_CS_PIN 21
#define LCD_CLK_PIN 18
#define LCD_MOSI_PIN 19

#define TOUCH_SDA_PIN 34
#define TOUCH_SCL_PIN 35
#define TOUCH_RST_PIN 24

#define FT6336U_ADDR 0x38

#define ST7796S_SLPOUT 0x11
#define ST7796S_INVON 0x21
#define ST7796S_DISPON 0x29
#define ST7796S_CASET 0x2A
#define ST7796S_RASET 0x2B
#define ST7796S_RAMWR 0x2C
#define ST7796S_MADCTL 0x36
#define ST7796S_COLMOD 0x3A

#define PRESENT_CHUNK_LINES 8

static uint16_t frame_buffer[TIPSY_LCD_WIDTH * TIPSY_LCD_HEIGHT];
static uint8_t flush_line_buffer[TIPSY_LCD_WIDTH * PRESENT_CHUNK_LINES * 2];

static void lcd_write_command(uint8_t cmd) {
  gpio_put(LCD_DC_PIN, 0);
  gpio_put(LCD_CS_PIN, 0);
  spi_write_blocking(spi0, &cmd, 1);
  gpio_put(LCD_CS_PIN, 1);
}

static void lcd_write_data(uint8_t data) {
  gpio_put(LCD_DC_PIN, 1);
  gpio_put(LCD_CS_PIN, 0);
  spi_write_blocking(spi0, &data, 1);
  gpio_put(LCD_CS_PIN, 1);
}

void tipsy_hal_init_stdio_and_led(void) {
  stdio_init_all();
  stdio_usb_init();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  sleep_ms(1000);
  for (int i = 0; i < 50; ++i) {
    if (stdio_usb_connected()) {
      break;
    }
    sleep_ms(100);
  }
}

void tipsy_display_init(void) {
  spi_init(spi0, 48000000);
  gpio_set_function(LCD_CLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);

  gpio_init(LCD_RST_PIN);
  gpio_set_dir(LCD_RST_PIN, GPIO_OUT);
  gpio_init(LCD_DC_PIN);
  gpio_set_dir(LCD_DC_PIN, GPIO_OUT);
  gpio_init(LCD_CS_PIN);
  gpio_set_dir(LCD_CS_PIN, GPIO_OUT);
  gpio_init(LCD_BL_PIN);
  gpio_set_dir(LCD_BL_PIN, GPIO_OUT);

  gpio_put(LCD_CS_PIN, 1);
  gpio_put(LCD_DC_PIN, 0);
  gpio_put(LCD_BL_PIN, 1);

  gpio_put(LCD_RST_PIN, 1);
  sleep_ms(100);
  gpio_put(LCD_RST_PIN, 0);
  sleep_ms(100);
  gpio_put(LCD_RST_PIN, 1);
  sleep_ms(100);

  lcd_write_command(0xF0);
  lcd_write_data(0xC3);
  lcd_write_command(0xF0);
  lcd_write_data(0x96);
  lcd_write_command(ST7796S_MADCTL);
  lcd_write_data(0x48);
  lcd_write_command(ST7796S_COLMOD);
  lcd_write_data(0x05);
  lcd_write_command(0xB4);
  lcd_write_data(0x01);
  lcd_write_command(0xB7);
  lcd_write_data(0xC6);
  lcd_write_command(0xE8);
  lcd_write_data(0x40);
  lcd_write_data(0x8A);
  lcd_write_data(0x00);
  lcd_write_data(0x00);
  lcd_write_data(0x29);
  lcd_write_data(0x19);
  lcd_write_data(0xA5);
  lcd_write_data(0x33);
  lcd_write_command(0xC5);
  lcd_write_data(0x16);
  lcd_write_command(0xE0);
  lcd_write_data(0xF0);
  lcd_write_data(0x09);
  lcd_write_data(0x0B);
  lcd_write_data(0x06);
  lcd_write_data(0x04);
  lcd_write_data(0x15);
  lcd_write_data(0x2F);
  lcd_write_data(0x54);
  lcd_write_data(0x42);
  lcd_write_data(0x3C);
  lcd_write_data(0x17);
  lcd_write_data(0x14);
  lcd_write_data(0x18);
  lcd_write_data(0x1B);
  lcd_write_command(0xE1);
  lcd_write_data(0xF0);
  lcd_write_data(0x09);
  lcd_write_data(0x0B);
  lcd_write_data(0x06);
  lcd_write_data(0x04);
  lcd_write_data(0x03);
  lcd_write_data(0x2D);
  lcd_write_data(0x43);
  lcd_write_data(0x42);
  lcd_write_data(0x3B);
  lcd_write_data(0x16);
  lcd_write_data(0x14);
  lcd_write_data(0x17);
  lcd_write_data(0x1B);
  lcd_write_command(0xF0);
  lcd_write_data(0x3C);
  lcd_write_command(0xF0);
  lcd_write_data(0x69);
  lcd_write_command(ST7796S_SLPOUT);
  sleep_ms(120);
  lcd_write_command(ST7796S_INVON);
  lcd_write_command(ST7796S_DISPON);
  sleep_ms(20);

  memset(frame_buffer, 0, sizeof(frame_buffer));

  printf("Display initialized (ST7796S)\r\n");
}

static void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1,
                               uint16_t y1) {
  uint8_t cmd;
  uint8_t data[4];

  cmd = ST7796S_CASET;
  data[0] = x0 >> 8;
  data[1] = x0 & 0xFF;
  data[2] = x1 >> 8;
  data[3] = x1 & 0xFF;
  gpio_put(LCD_CS_PIN, 0);
  gpio_put(LCD_DC_PIN, 0);
  spi_write_blocking(spi0, &cmd, 1);
  gpio_put(LCD_DC_PIN, 1);
  spi_write_blocking(spi0, data, 4);
  gpio_put(LCD_CS_PIN, 1);
  busy_wait_us(1);

  cmd = ST7796S_RASET;
  data[0] = y0 >> 8;
  data[1] = y0 & 0xFF;
  data[2] = y1 >> 8;
  data[3] = y1 & 0xFF;
  gpio_put(LCD_CS_PIN, 0);
  gpio_put(LCD_DC_PIN, 0);
  spi_write_blocking(spi0, &cmd, 1);
  gpio_put(LCD_DC_PIN, 1);
  spi_write_blocking(spi0, data, 4);
  gpio_put(LCD_CS_PIN, 1);
  busy_wait_us(1);

  cmd = ST7796S_RAMWR;
  gpio_put(LCD_CS_PIN, 0);
  gpio_put(LCD_DC_PIN, 0);
  spi_write_blocking(spi0, &cmd, 1);
  gpio_put(LCD_CS_PIN, 1);
  busy_wait_us(1);
}

void tipsy_display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                             uint16_t color) {
  uint16_t *row = NULL;

  if (w == 0 || h == 0 || x >= TIPSY_LCD_WIDTH || y >= TIPSY_LCD_HEIGHT) {
    return;
  }
  if (x + w > TIPSY_LCD_WIDTH) {
    w = TIPSY_LCD_WIDTH - x;
  }
  if (y + h > TIPSY_LCD_HEIGHT) {
    h = TIPSY_LCD_HEIGHT - y;
  }

  for (uint16_t row_idx = 0; row_idx < h; ++row_idx) {
    row = &frame_buffer[(uint32_t)(y + row_idx) * TIPSY_LCD_WIDTH + x];
    for (uint16_t col_idx = 0; col_idx < w; ++col_idx) {
      row[col_idx] = color;
    }
  }
}

void tipsy_display_fill_screen(uint16_t color) {
  tipsy_display_fill_rect(0, 0, TIPSY_LCD_WIDTH, TIPSY_LCD_HEIGHT, color);
}

void tipsy_display_present(void) {
  display_set_window(0, 0, TIPSY_LCD_WIDTH - 1, TIPSY_LCD_HEIGHT - 1);

  gpio_put(LCD_DC_PIN, 1);
  gpio_put(LCD_CS_PIN, 0);

  for (uint16_t y = 0; y < TIPSY_LCD_HEIGHT; y += PRESENT_CHUNK_LINES) {
    const uint16_t lines =
        (uint16_t)((y + PRESENT_CHUNK_LINES <= TIPSY_LCD_HEIGHT)
                       ? PRESENT_CHUNK_LINES
                       : (TIPSY_LCD_HEIGHT - y));
    size_t out_index = 0;

    for (uint16_t row = 0; row < lines; ++row) {
      const uint16_t *src =
          &frame_buffer[(uint32_t)(y + row) * TIPSY_LCD_WIDTH];
      for (uint16_t x = 0; x < TIPSY_LCD_WIDTH; ++x) {
        flush_line_buffer[out_index++] = (uint8_t)(src[x] >> 8);
        flush_line_buffer[out_index++] = (uint8_t)(src[x] & 0xFF);
      }
    }

    spi_write_blocking(spi0, flush_line_buffer, out_index);
  }

  gpio_put(LCD_CS_PIN, 1);
  busy_wait_us(1);
}

static const uint8_t *get_glyph_5x7(char c) {
  static const uint8_t glyph_space[7] = {0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00};
  static const uint8_t glyph_A[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
  static const uint8_t glyph_B[7] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
  static const uint8_t glyph_C[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
  static const uint8_t glyph_D[7] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
  static const uint8_t glyph_E[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
  static const uint8_t glyph_F[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
  static const uint8_t glyph_G[7] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E};
  static const uint8_t glyph_H[7] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
  static const uint8_t glyph_I[7] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
  static const uint8_t glyph_K[7] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
  static const uint8_t glyph_L[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
  static const uint8_t glyph_M[7] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
  static const uint8_t glyph_N[7] = {0x11, 0x19, 0x19, 0x15, 0x13, 0x13, 0x11};
  static const uint8_t glyph_O[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
  static const uint8_t glyph_P[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
  static const uint8_t glyph_R[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
  static const uint8_t glyph_S[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
  static const uint8_t glyph_T[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
  static const uint8_t glyph_U[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
  static const uint8_t glyph_V[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
  static const uint8_t glyph_Y[7] = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
  static const uint8_t glyph_b[7] = {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x1E};
  static const uint8_t glyph_a[7] = {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F};
  static const uint8_t glyph_c[7] = {0x00, 0x00, 0x0E, 0x10, 0x10, 0x10, 0x0E};
  static const uint8_t glyph_d[7] = {0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F};
  static const uint8_t glyph_e[7] = {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E};
  static const uint8_t glyph_f[7] = {0x06, 0x08, 0x08, 0x1E, 0x08, 0x08, 0x08};
  static const uint8_t glyph_g[7] = {0x00, 0x00, 0x0F, 0x11, 0x0F, 0x01, 0x0E};
  static const uint8_t glyph_h[7] = {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x11};
  static const uint8_t glyph_i[7] = {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E};
  static const uint8_t glyph_k[7] = {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12};
  static const uint8_t glyph_l[7] = {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
  static const uint8_t glyph_m[7] = {0x00, 0x00, 0x1A, 0x15, 0x15, 0x11, 0x11};
  static const uint8_t glyph_n[7] = {0x00, 0x00, 0x1A, 0x15, 0x11, 0x11, 0x11};
  static const uint8_t glyph_o[7] = {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E};
  static const uint8_t glyph_p[7] = {0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10};
  static const uint8_t glyph_r[7] = {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10};
  static const uint8_t glyph_s[7] = {0x00, 0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E};
  static const uint8_t glyph_t[7] = {0x08, 0x08, 0x1C, 0x08, 0x08, 0x09, 0x06};
  static const uint8_t glyph_u[7] = {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D};
  static const uint8_t glyph_v[7] = {0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04};
  static const uint8_t glyph_w[7] = {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A};
  static const uint8_t glyph_y[7] = {0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E};
  static const uint8_t glyph_0[7] = {0x0E, 0x13, 0x15, 0x15, 0x19, 0x11, 0x0E};
  static const uint8_t glyph_1[7] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
  static const uint8_t glyph_2[7] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
  static const uint8_t glyph_3[7] = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
  static const uint8_t glyph_4[7] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
  static const uint8_t glyph_5[7] = {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
  static const uint8_t glyph_6[7] = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
  static const uint8_t glyph_7[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
  static const uint8_t glyph_8[7] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
  static const uint8_t glyph_colon[7] = {0x00, 0x04, 0x04, 0x00,
                                         0x04, 0x04, 0x00};
  static const uint8_t glyph_dash[7] = {0x00, 0x00, 0x00, 0x1F,
                                        0x00, 0x00, 0x00};
  static const uint8_t glyph_gt[7] = {0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10};
  static const uint8_t glyph_paren_l[7] = {0x02, 0x04, 0x08, 0x08,
                                           0x08, 0x04, 0x02};
  static const uint8_t glyph_paren_r[7] = {0x08, 0x04, 0x02, 0x02,
                                           0x02, 0x04, 0x08};

  switch (c) {
    case 'A': return glyph_A;
    case 'B': return glyph_B;
    case 'C': return glyph_C;
    case 'D': return glyph_D;
    case 'E': return glyph_E;
    case 'F': return glyph_F;
    case 'G': return glyph_G;
    case 'H': return glyph_H;
    case 'I': return glyph_I;
    case 'K': return glyph_K;
    case 'L': return glyph_L;
    case 'M': return glyph_M;
    case 'N': return glyph_N;
    case 'O': return glyph_O;
    case 'P': return glyph_P;
    case 'R': return glyph_R;
    case 'S': return glyph_S;
    case 'T': return glyph_T;
    case 'U': return glyph_U;
    case 'V': return glyph_V;
    case 'Y': return glyph_Y;
    case 'b': return glyph_b;
    case 'a': return glyph_a;
    case 'c': return glyph_c;
    case 'd': return glyph_d;
    case 'e': return glyph_e;
    case 'f': return glyph_f;
    case 'g': return glyph_g;
    case 'h': return glyph_h;
    case 'i': return glyph_i;
    case 'k': return glyph_k;
    case 'l': return glyph_l;
    case 'm': return glyph_m;
    case 'n': return glyph_n;
    case 'o': return glyph_o;
    case 'p': return glyph_p;
    case 'r': return glyph_r;
    case 's': return glyph_s;
    case 't': return glyph_t;
    case 'u': return glyph_u;
    case 'v': return glyph_v;
    case 'w': return glyph_w;
    case 'y': return glyph_y;
    case '0': return glyph_0;
    case '1': return glyph_1;
    case '2': return glyph_2;
    case '3': return glyph_3;
    case '4': return glyph_4;
    case '5': return glyph_5;
    case '6': return glyph_6;
    case '7': return glyph_7;
    case '8': return glyph_8;
    case ':': return glyph_colon;
    case '-': return glyph_dash;
    case '>': return glyph_gt;
    case '(': return glyph_paren_l;
    case ')': return glyph_paren_r;
    case ' ': return glyph_space;
    default: return glyph_space;
  }
}

static void draw_char_5x7(uint16_t x, uint16_t y, char c, uint16_t color,
                          uint8_t scale) {
  const uint8_t *glyph = get_glyph_5x7(c);

  for (uint8_t row = 0; row < 7; ++row) {
    for (uint8_t col = 0; col < 5; ++col) {
      if (glyph[row] & (1u << (4 - col))) {
        tipsy_display_fill_rect(x + (col * scale), y + (row * scale), scale,
                                scale, color);
      }
    }
  }
}

void tipsy_display_draw_text_5x7(uint16_t x, uint16_t y, const char *text,
                                 uint16_t color, uint8_t scale) {
  if (text == NULL) {
    return;
  }

  while (*text != '\0') {
    draw_char_5x7(x, y, *text, color, scale);
    x += 6 * scale;
    ++text;
  }
}

void tipsy_touch_init(void) {
  i2c_init(i2c1, 400000);
  gpio_set_function(TOUCH_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(TOUCH_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(TOUCH_SDA_PIN);
  gpio_pull_up(TOUCH_SCL_PIN);

  gpio_init(TOUCH_RST_PIN);
  gpio_set_dir(TOUCH_RST_PIN, GPIO_OUT);

  gpio_put(TOUCH_RST_PIN, 1);
  sleep_ms(10);
  gpio_put(TOUCH_RST_PIN, 0);
  sleep_ms(10);
  gpio_put(TOUCH_RST_PIN, 1);
  sleep_ms(300);

  printf("Touch initialized (FT6336U on i2c1)\r\n");
}

bool tipsy_touch_read(TipsyTouchSample *sample) {
  uint8_t data[4];
  uint8_t reg = 0x02;
  uint8_t td_status = 0;
  const uint32_t timeout_us = 5000;

  if (sample == NULL) {
    return false;
  }

  sample->pressed = false;
  sample->x = 0;
  sample->y = 0;

  if (i2c_write_timeout_us(i2c1, FT6336U_ADDR, &reg, 1, true, timeout_us) < 0) {
    return false;
  }
  if (i2c_read_timeout_us(i2c1, FT6336U_ADDR, &td_status, 1, false,
                          timeout_us) < 0) {
    return false;
  }
  if ((td_status & 0x0F) == 0) {
    return false;
  }

  reg = 0x03;
  if (i2c_write_timeout_us(i2c1, FT6336U_ADDR, &reg, 1, true, timeout_us) < 0) {
    return false;
  }
  if (i2c_read_timeout_us(i2c1, FT6336U_ADDR, data, 4, false, timeout_us) < 0) {
    return false;
  }

  sample->pressed = true;
  sample->x = ((data[0] & 0x0F) << 8) | data[1];
  sample->y = ((data[2] & 0x0F) << 8) | data[3];
  return true;
}

bool tipsy_rect_contains(const TipsyRect *rect, uint16_t x, uint16_t y) {
  if (rect == NULL) {
    return false;
  }
  return x >= rect->x && x < rect->x + rect->w && y >= rect->y &&
         y < rect->y + rect->h;
}
