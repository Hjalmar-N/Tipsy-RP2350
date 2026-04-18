# Waveshare reference files for RP2350-Touch-LCD-3.5

This folder contains official or near-official reference material used only to extract VERIFIED hardware facts for the Waveshare RP2350-Touch-LCD-3.5 board.

## Purpose

These files are not the new application architecture.

They are only here to help verify:

- display controller
- touch controller
- display bus type
- board pin mapping
- reset / dc / cs / backlight handling
- minimal official initialization flow

## Files currently included

- `sample_code.txt`  
  Instruction file for AI assistants describing how these references must be used.

- `LVGL_example.c`  
  Official LVGL example showing DMA-based display flush, SPI-based LCD transfer, and touch callback flow.

- `LCD_3in5.c`  
  Official display driver source with reset flow, command/data writes, init sequence, scan direction handling, and display window functions.

- `LCD_3in5_test.c`  
  Official test/demo program showing practical display init, backlight enable, touch init, and example usage.

- `lv_conf.h`  
  LVGL configuration used by the example code.

- `waveshare_rp2350_touch_lcd_3.5.h`  
  Board header with verified Pico board defaults such as UART/I2C defaults and flash size.

## Important limitations

The current reference set is useful, but still incomplete.

The most important missing file is the board config / pin-definition file, such as:

- `DEV_Config.h`
- `DEV_Config.c`
- `LCD_3in5.h`
- `FT6336U.h`
- or an equivalent file that defines:
  - `LCD_DC_PIN`
  - `LCD_CS_PIN`
  - `LCD_RST_PIN`
  - `SPI_PORT`
  - `Touch_INT_PIN`
  - backlight pin / PWM pin

Without that file, exact GPIO mapping for the onboard LCD and touch controller is still not fully verified.

## Rules for use

- Do not guess hardware details.
- Do not use generic “typical 3.5 inch TFT” assumptions.
- Do not invent GPIO mappings.
- Do not replace verified Waveshare facts with generic Pico examples.
- Preserve the current USB serial heartbeat in the project.
- Prefer the smallest possible verified next step.

## Current verified facts

From official Waveshare material and uploaded example files:

- Display controller: ST7796
- Touch controller: FT6336
- Resolution: 320x480
- Display path used in examples: SPI-based transfer path
- Touch path used in examples: I2C-based touch controller
- LVGL example uses DMA for LCD pixel transfer
- LVGL example uses `Touch_INT_PIN` + `FT6336U_Get_Point()`
- LCD driver contains the actual display init sequence
- LCD test example initializes display in vertical mode and sets backlight with PWM

## Recommended next missing file to add

Please add the board pin-definition/config file from the official Waveshare repo next.
That is the most important remaining file for verified display bring-up.