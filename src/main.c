/**
 * Tipsy-RP2350 - Minimal bring-up
 *
 * First verification step:
 * - USB serial heartbeat output
 * - Built-in LED blink as backup visual confirmation
 *
 * No board-specific hardware assumptions yet.
 */

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdio.h>


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

  printf("Display initialized (ST7796S 320x480)\n");
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

  // Fill with color (RGB565 format, high byte first per Waveshare)
  uint8_t color_buf[2];
  color_buf[0] = color >> 8;   // High byte first
  color_buf[1] = color & 0xFF; // Low byte second

  gpio_put(LCD_DC_PIN, 1); // Data mode
  gpio_put(LCD_CS_PIN, 0); // Select

  for (uint32_t i = 0; i < w * h; i++) {
    spi_write_blocking(spi0, color_buf, 2);
  }

  gpio_put(LCD_CS_PIN, 1); // Deselect
}

static void display_fill_screen(uint16_t color) {
  display_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

static void touch_init(void) {
  // Initialize I2C0 at 100kHz (verified from Waveshare DEV_Config.c)
  i2c_init(i2c0, 100000);
  gpio_set_function(TOUCH_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(TOUCH_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(TOUCH_SDA_PIN);
  gpio_pull_up(TOUCH_SCL_PIN);

  // Initialize reset pin
  gpio_init(TOUCH_RST_PIN);
  gpio_set_dir(TOUCH_RST_PIN, GPIO_OUT);

  // Hardware reset (from Waveshare FT6336U_Reset)
  gpio_put(TOUCH_RST_PIN, 0);
  sleep_ms(100);
  gpio_put(TOUCH_RST_PIN, 1);
  sleep_ms(100);

  printf("Touch initialized (FT6336U)\n");
}

static bool touch_read(uint16_t *x, uint16_t *y) {
  uint8_t data[4];
  uint8_t reg = FT6336U_TD_STATUS;

  // Read touch status
  i2c_write_blocking(i2c0, FT6336U_ADDR, &reg, 1, true);
  i2c_read_blocking(i2c0, FT6336U_ADDR, data, 1, false);

  uint8_t touch_points = data[0] & 0x0F;
  if (touch_points == 0) {
    return false; // No touch
  }

  // Read X coordinate (P1_XH and P1_XL)
  reg = FT6336U_P1_XH;
  i2c_write_blocking(i2c0, FT6336U_ADDR, &reg, 1, true);
  i2c_read_blocking(i2c0, FT6336U_ADDR, data, 4, false);

  *x = ((data[0] & 0x0F) << 8) | data[1];
  *y = ((data[2] & 0x0F) << 8) | data[3];

  return true;
}

static void display_draw_static_layout(void) {
  printf("Drawing static layout...\n");

  // Background
  display_fill_screen(COLOR_DARKGRAY);

  // Header bar (top 60px)
  display_fill_rect(0, 0, LCD_WIDTH, 60, COLOR_LIGHTGRAY);

  // Three large drink button areas (simplified layout)
  // Button 1: Red drink
  display_fill_rect(20, 80, 280, 100, COLOR_RED);

  // Button 2: Green drink
  display_fill_rect(20, 200, 280, 100, COLOR_GREEN);

  // Button 3: Blue drink
  display_fill_rect(20, 320, 280, 100, COLOR_BLUE);

  printf("Static layout complete\n");
}

int main() {
  // Initialize stdio for USB serial output
  stdio_init_all();

  // Initialize LED pin
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // Wait for USB serial connection (optional, helps with early messages)
  sleep_ms(2000);

  printf("Tipsy-RP2350 boot\n");
  printf("Board: Waveshare RP2350-Touch-LCD-3.5\n");
  printf("SDK: Pico SDK 2.x\n");

  // Initialize and test display
  printf("Initializing display...\n");
  display_init();

  // Draw static layout
  display_draw_static_layout();

  // Initialize touch
  printf("Initializing touch...\n");
  touch_init();

  uint32_t count = 0;
  uint16_t touch_x = 0, touch_y = 0;
  bool last_touch_state = false;

  while (true) {
    // Toggle LED
    gpio_put(LED_PIN, 1);
    sleep_ms(250);
    gpio_put(LED_PIN, 0);
    sleep_ms(250);

    // Poll touch status
    bool touch_detected = touch_read(&touch_x, &touch_y);

    // Log touch events (only on state change to reduce spam)
    if (touch_detected && !last_touch_state) {
      printf("Touch DOWN at (%u, %u)\n", touch_x, touch_y);
    } else if (!touch_detected && last_touch_state) {
      printf("Touch UP\n");
    }
    last_touch_state = touch_detected;

    // Heartbeat every second
    if (count % 2 == 0) {
      printf("Heartbeat: %lu\n", count / 2);
    }

    count++;
  }

  return 0;
}
