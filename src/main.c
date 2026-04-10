/**
 * Tipsy-RP2350 - USB serial isolation test
 *
 * Goal:
 * - Verify USB serial output independently of display/touch
 * - Keep LED blink as visual confirmation
 *
 * Display/touch code is left in the file but not used from main().
 */

#include <stdbool.h>
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

// ST7796S Display pins (verified from Waveshare DEV_Config.h)
#define LCD_RST_PIN 23
#define LCD_DC_PIN 20
#define LCD_BL_PIN 22
#define LCD_CS_PIN 21
#define LCD_CLK_PIN 18
#define LCD_MOSI_PIN 19

// ST7796S commands (from Waveshare LCD_3in5.h)
#define ST7796S_SWRESET 0x01
#define ST7796S_SLPOUT 0x11
#define ST7796S_INVON 0x21
#define ST7796S_DISPON 0x29
#define ST7796S_CASET 0x2A
#define ST7796S_RASET 0x2B
#define ST7796S_RAMWR 0x2C
#define ST7796S_MADCTL 0x36
#define ST7796S_COLMOD 0x3A

#define LCD_WIDTH 320
#define LCD_HEIGHT 480

// FT6336U Touch pins (verified from Waveshare DEV_Config.h)
#define TOUCH_SDA_PIN 34
#define TOUCH_SCL_PIN 35
#define TOUCH_RST_PIN 24
#define TOUCH_INT_PIN 25

// FT6336U I2C address and registers (from Waveshare FT6336U.h)
#define FT6336U_ADDR 0x38
#define FT6336U_TD_STATUS 0x02
#define FT6336U_P1_XH 0x03
#define FT6336U_P1_XL 0x04
#define FT6336U_P1_YH 0x05
#define FT6336U_P1_YL 0x06

// RGB565 colors
#define COLOR_BLACK 0x0000
#define COLOR_DARKGRAY 0x39E7
#define COLOR_LIGHTGRAY 0xBDF7
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_ORANGE 0xFD20
#define COLOR_PURPLE 0x780F
#define COLOR_CYAN 0x07FF

static void lcd_write_command(uint8_t cmd) {
  gpio_put(LCD_DC_PIN, 0); // Command mode
  gpio_put(LCD_CS_PIN, 0); // Select
  spi_write_blocking(spi0, &cmd, 1);
  gpio_put(LCD_CS_PIN, 1); // Deselect
}

static void lcd_write_data(uint8_t data) {
  gpio_put(LCD_DC_PIN, 1); // Data mode
  gpio_put(LCD_CS_PIN, 0); // Select
  spi_write_blocking(spi0, &data, 1);
  gpio_put(LCD_CS_PIN, 1); // Deselect
}

static void display_init(void) {
  // Initialize SPI0 at 32MHz (verified from Waveshare DEV_Config.c)
  spi_init(spi0, 32000000);
  gpio_set_function(LCD_CLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);

  // Initialize control pins
  gpio_init(LCD_RST_PIN);
  gpio_set_dir(LCD_RST_PIN, GPIO_OUT);
  gpio_init(LCD_DC_PIN);
  gpio_set_dir(LCD_DC_PIN, GPIO_OUT);
  gpio_init(LCD_CS_PIN);
  gpio_set_dir(LCD_CS_PIN, GPIO_OUT);
  gpio_init(LCD_BL_PIN);
  gpio_set_dir(LCD_BL_PIN, GPIO_OUT);

  // Default states
  gpio_put(LCD_CS_PIN, 1);
  gpio_put(LCD_DC_PIN, 0);
  gpio_put(LCD_BL_PIN, 1); // Backlight on

  // Hardware reset (from Waveshare LCD_3IN5_Reset)
  gpio_put(LCD_RST_PIN, 1);
  sleep_ms(100);
  gpio_put(LCD_RST_PIN, 0);
  sleep_ms(100);
  gpio_put(LCD_RST_PIN, 1);
  sleep_ms(100);

  // ST7796S initialization sequence (from Waveshare LCD_3IN5_Init)
  lcd_write_command(0xF0);
  lcd_write_data(0xC3);

  lcd_write_command(0xF0);
  lcd_write_data(0x96);

  lcd_write_command(ST7796S_MADCTL);
  lcd_write_data(0x48); // MY=0, MX=1, MV=0, ML=0, BGR=1

  lcd_write_command(ST7796S_COLMOD);
  lcd_write_data(0x05); // 16-bit RGB565

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

  printf("Display initialized (ST7796S 320x480)\r\n");
}

static void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1,
                               uint16_t y1) {
  lcd_write_command(ST7796S_CASET);
  lcd_write_data(x0 >> 8);
  lcd_write_data(x0 & 0xFF);
  lcd_write_data(x1 >> 8);
  lcd_write_data(x1 & 0xFF);

  lcd_write_command(ST7796S_RASET);
  lcd_write_data(y0 >> 8);
  lcd_write_data(y0 & 0xFF);
  lcd_write_data(y1 >> 8);
  lcd_write_data(y1 & 0xFF);

  lcd_write_command(ST7796S_RAMWR);
}

static void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                              uint16_t color) {
  if (w == 0 || h == 0)
    return;
  if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
    return;
  if (x + w > LCD_WIDTH)
    w = LCD_WIDTH - x;
  if (y + h > LCD_HEIGHT)
    h = LCD_HEIGHT - y;

  display_set_window(x, y, x + w - 1, y + h - 1);

  uint8_t color_buf[2];
  color_buf[0] = color >> 8;
  color_buf[1] = color & 0xFF;

  gpio_put(LCD_DC_PIN, 1);
  gpio_put(LCD_CS_PIN, 0);

  for (uint32_t i = 0; i < w * h; i++) {
    spi_write_blocking(spi0, color_buf, 2);
  }

  gpio_put(LCD_CS_PIN, 1);
}

static void display_fill_screen(uint16_t color) {
  display_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

static void touch_init(void) {
  i2c_init(i2c0, 100000);
  gpio_set_function(TOUCH_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(TOUCH_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(TOUCH_SDA_PIN);
  gpio_pull_up(TOUCH_SCL_PIN);

  gpio_init(TOUCH_RST_PIN);
  gpio_set_dir(TOUCH_RST_PIN, GPIO_OUT);

  gpio_put(TOUCH_RST_PIN, 0);
  sleep_ms(100);
  gpio_put(TOUCH_RST_PIN, 1);
  sleep_ms(100);

  printf("Touch initialized (FT6336U)\r\n");
}

static bool touch_read(uint16_t *x, uint16_t *y) {
  uint8_t data[4];
  uint8_t reg = FT6336U_TD_STATUS;

  int written = i2c_write_blocking(i2c0, FT6336U_ADDR, &reg, 1, true);
  if (written < 0)
    return false;

  int read = i2c_read_blocking(i2c0, FT6336U_ADDR, data, 1, false);
  if (read < 0)
    return false;

  uint8_t touch_points = data[0] & 0x0F;
  if (touch_points == 0) {
    return false;
  }

  reg = FT6336U_P1_XH;
  written = i2c_write_blocking(i2c0, FT6336U_ADDR, &reg, 1, true);
  if (written < 0)
    return false;

  read = i2c_read_blocking(i2c0, FT6336U_ADDR, data, 4, false);
  if (read < 0)
    return false;

  *x = ((data[0] & 0x0F) << 8) | data[1];
  *y = ((data[2] & 0x0F) << 8) | data[3];

  return true;
}

static void display_draw_static_layout(void) {
  printf("Drawing static layout...\r\n");

  display_fill_screen(COLOR_DARKGRAY);
  display_fill_rect(0, 0, LCD_WIDTH, 60, COLOR_LIGHTGRAY);
  display_fill_rect(20, 80, 280, 100, COLOR_RED);
  display_fill_rect(20, 200, 280, 100, COLOR_GREEN);
  display_fill_rect(20, 320, 280, 100, COLOR_BLUE);

  printf("Static layout complete\r\n");
}

int main() {
  stdio_init_all();
  stdio_usb_init();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // Give USB time to enumerate
  sleep_ms(1000);

  // Wait a short time for terminal connection, but do not block forever
  for (int i = 0; i < 50; i++) {
    if (stdio_usb_connected()) {
      break;
    }
    sleep_ms(100);
  }

  printf("\r\nUSB serial isolation test start\r\n");
  printf("If you can read this, USB serial works.\r\n");
  fflush(stdout);

  while (true) {
    gpio_put(LED_PIN, 1);
    printf("alive\r\n");
    fflush(stdout);
    sleep_ms(500);

    gpio_put(LED_PIN, 0);
    printf("alive\r\n");
    fflush(stdout);
    sleep_ms(500);
  }

  return 0;
}