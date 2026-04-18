# Tipsy-RP2350

Tipsy-RP2350 is a new RP2350-based drink machine project for the Waveshare RP2350-Touch-LCD-3.5 board.

## Purpose

This project is a fresh hardware/software track intended to explore RP2350 as a serious platform for the Tipsy drink machine.

The priorities are:

1. reliable hardware bring-up
2. safe and deterministic pump control
3. clean architecture
4. testable UI and recipe flow
5. stepwise progress toward a real product

## Hardware target

* Board: Waveshare RP2350-Touch-LCD-3.5
* MCU: RP2350
* Language: C/C++
* SDK: Pico SDK 2.x or newer

## Current status

This project is in early bring-up, but core UI hardware paths are verified.

**Verified Hardware Subsystems & Checkpoint:**
* **USB Serial**: Fully functional.
* **Display (ST7796S)**: SPI `spi0` works. Init, clearing, and layout drawing verified.
* **Touch (FT6336U)**: I2C works, coordinates and interactions logged via serial.
  * Hardware constraint: touch I2C pins (GPIO 34/35) must use hardware `i2c1` at 400kHz.
  * Timing requirements: reset sequence requires precise timings (10ms low, 300ms high before polling).
* **UI Base (Verified Checkpoint)**:
  * display + touch fungerar
  * touch fungerar via `i2c1`
  * appen har nu top-level meny med `Drinks`, `Shots`, `Settings`
  * `Drinks` leder till drinklista och vidare till befintlig detail-vy
  * shot-menyn genereras nu från tillgängliga alkoholhaltiga ingredients
  * shot-menyn följer enkel availability-logik och visar inte shots utan giltig ingredient/pump-mappning
  * befintligt detail-flöde med `40/60/80` ml-val och simulerad pouring återanvänds för valda shots och drinks
  * `Settings` är fortfarande en enkel placeholder-skärm med `Back`
  * detta är nästa strukturella checkpoint i UI-logiken
  * en liten intern logikmodell finns för drinks, ingredients, pumps och app-state
  * fyra första drinkar finns representerade i koden: `GT`, `Negroni`, `Aperol`, `Limoncello`
  * enkel availability-logik och en fiktiv pour-tidsmodell finns som grund för nästa steg

Expected major subsystems:

* display
* touch
* pump control
* recipe logic
* settings/storage
* safety handling
* cleaning mode

## UI availability

The drink menu now follows the same simple availability direction as the shot menu.

* Drinks are hidden when a required ingredient is disabled, missing a pump mapping, or no longer resolves to a valid enabled pump.
* Drinks appear again automatically when ingredient availability and mapping are restored.
* Drink availability is also reported over USB serial when the drink menu is drawn, so hidden/shown decisions can be verified without adding new UI.
* The shot menu is generated from alcoholic ingredients only, follows the same availability rules, and reports shown/hidden decisions over USB serial when the shot menu is drawn.
* The settings menu now includes a simple pump mapping view and editor, so pump-to-ingredient mappings can be changed from the UI and existing availability logic can react on the next menu redraw.
* A first calibration view now exists in settings: a pump can be selected, a simulated test can be run, and a placeholder calibration value can be saved from the UI.
* Drink lists, shot lists, and relevant settings lists now use simple vertical touch-scroll instead of `Next` / `Prev` paging, while tap-to-select remains unchanged for list items.
* Settings now uses a single scrollable list in the main settings view, with pump rows plus `Calibration` and `Back` as normal list items instead of separate panels/buttons.
* The main settings list is currently built from six pump rows followed by `Calibration` and `Back`, so all expected settings entries remain reachable through the same scroll list.
* The scrollable settings mapping list still uses one continuous item sequence rather than internal page splits, so every ingredient can be reached by scrolling.
* In this step, the calibration test still uses simulated placeholder values rather than a real measured pump result.
* The calibration screen now uses shorter labels so `Trial` and `Saved` values stay readable with the current font.
* Real pump control and the final calibration method remain later steps.

## Development rules

* Prefer small, testable changes
* Do not assume hardware works without verification
* Keep app logic separate from hardware-specific code
* Prioritize safety and deterministic behavior

## Build

Use:

```bash
./pre-flight.sh
```

Or manually:

```bash
cmake -S . -B build -DPICO_BOARD=pico2
cmake --build build -j
```

## Notes for AI assistants

* Do not inspect `references/` unless explicitly asked
* Prefer minimal diffs
* Show proposed changes before applying them
* Keep bring-up simple before adding optimization
