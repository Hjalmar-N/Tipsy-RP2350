/**
 * Tipsy-RP2350 - Minimal bring-up
 * 
 * First verification step:
 * - USB serial heartbeat output
 * - Built-in LED blink as backup visual confirmation
 * 
 * No board-specific hardware assumptions yet.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

// ST7796S Display pins (verified from Waveshare DEV_Config.h)
#define LCD_RST_PIN     23
#define LCD_DC_PIN      20
#define LCD_BL_PIN      22
#define LCD_CS_PIN      21
#define LCD_CLK_PIN     18
#define LCD_MOSI_PIN    19

// ST7796S commands (from Waveshare LCD_3in5.h)
#define ST7796S_SWRESET 0x01
#define ST7796S_SLPOUT  0x11
#define ST7796S_DISPON  0x29
#define ST7796S_CASET   0x2A
#define ST7796S_RASET   0x2B
#define ST7796S_RAMWR   0x2C
#define ST7796S_MADCTL  0x36
#define ST7796S_COLMOD  0x3A

#define LCD_WIDTH  320
#define LCD_HEIGHT 480

// RGB565 color
#define COLOR_RED   0xF800

static void lcd_write_command(uint8_t cmd) {
    gpio_put(LCD_DC_PIN, 0);  // Command mode
    gpio_put(LCD_CS_PIN, 0);  // Select
    spi_write_blocking(spi0, &cmd, 1);
    gpio_put(LCD_CS_PIN, 1);  // Deselect
}

static void lcd_write_data(uint8_t data) {
    gpio_put(LCD_DC_PIN, 1);  // Data mode
    gpio_put(LCD_CS_PIN, 0);  // Select
    spi_write_blocking(spi0, &data, 1);
    gpio_put(LCD_CS_PIN, 1);  // Deselect
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
    gpio_put(LCD_BL_PIN, 1);  // Backlight on
    
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
    lcd_write_data(0x48);  // MY=0, MX=1, MV=0, ML=0, BGR=1
    
    lcd_write_command(ST7796S_COLMOD);
    lcd_write_data(0x05);  // 16-bit RGB565
    
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
    
    lcd_write_command(ST7796S_DISPON);
    sleep_ms(20);
    
    printf("Display initialized (ST7796S 320x480)\n");
}

static void display_fill_screen(uint16_t color) {
    // Set window to full screen
    lcd_write_command(ST7796S_CASET);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data((LCD_WIDTH - 1) >> 8);
    lcd_write_data((LCD_WIDTH - 1) & 0xFF);
    
    lcd_write_command(ST7796S_RASET);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data((LCD_HEIGHT - 1) >> 8);
    lcd_write_data((LCD_HEIGHT - 1) & 0xFF);
    
    lcd_write_command(ST7796S_RAMWR);
    
    // Fill screen with color (RGB565 format, big-endian)
    uint8_t color_buf[2];
    color_buf[0] = color >> 8;
    color_buf[1] = color & 0xFF;
    
    gpio_put(LCD_DC_PIN, 1);  // Data mode
    gpio_put(LCD_CS_PIN, 0);  // Select
    
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        spi_write_blocking(spi0, color_buf, 2);
    }
    
    gpio_put(LCD_CS_PIN, 1);  // Deselect
    
    printf("Screen filled with color 0x%04X\n", color);
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
    display_fill_screen(COLOR_RED);
    printf("Display bring-up complete\n");
    
    uint32_t count = 0;
    
    while (true) {
        // Toggle LED
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
        
        // Heartbeat every second
        if (count % 2 == 0) {
            printf("Heartbeat: %lu\n", count / 2);
        }
        
        count++;
    }
    
    return 0;
}
