/**
 * Tipsy-RP2350 - Display + Touch Logging
 *
 * Goal:
 * - Read FT6336U touch coordinates over i2c1 using tested reset logic.
 * - Show static display layout on ST7796S.
 * - Log minimalist Touch DOWN, MOVE, and UP events.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

// ST7796S Display pins
#define LCD_RST_PIN 23
#define LCD_DC_PIN 20
#define LCD_BL_PIN 22
#define LCD_CS_PIN 21
#define LCD_CLK_PIN 18
#define LCD_MOSI_PIN 19

// FT6336U Touch pins
#define TOUCH_SDA_PIN 34
#define TOUCH_SCL_PIN 35
#define TOUCH_RST_PIN 24
#define TOUCH_INT_PIN 25

// Constants
#define LCD_WIDTH 320
#define LCD_HEIGHT 480
#define FT6336U_ADDR 0x38

// ST7796S Commands
#define ST7796S_SLPOUT 0x11
#define ST7796S_INVON 0x21
#define ST7796S_DISPON 0x29
#define ST7796S_CASET 0x2A
#define ST7796S_RASET 0x2B
#define ST7796S_RAMWR 0x2C
#define ST7796S_MADCTL 0x36
#define ST7796S_COLMOD 0x3A

// Colors (RGB565)
#define COLOR_BLACK 0x0000
#define COLOR_DARKGRAY 0x39E7
#define COLOR_LIGHTGRAY 0xBDF7
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_WHITE 0xFFFF

// Display functions
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

static void display_init(void) {
  spi_init(spi0, 48000000);
  gpio_set_function(LCD_CLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);

  gpio_init(LCD_RST_PIN); gpio_set_dir(LCD_RST_PIN, GPIO_OUT);
  gpio_init(LCD_DC_PIN); gpio_set_dir(LCD_DC_PIN, GPIO_OUT);
  gpio_init(LCD_CS_PIN); gpio_set_dir(LCD_CS_PIN, GPIO_OUT);
  gpio_init(LCD_BL_PIN); gpio_set_dir(LCD_BL_PIN, GPIO_OUT);

  gpio_put(LCD_CS_PIN, 1);
  gpio_put(LCD_DC_PIN, 0);
  gpio_put(LCD_BL_PIN, 1);

  gpio_put(LCD_RST_PIN, 1); sleep_ms(100);
  gpio_put(LCD_RST_PIN, 0); sleep_ms(100);
  gpio_put(LCD_RST_PIN, 1); sleep_ms(100);

  lcd_write_command(0xF0); lcd_write_data(0xC3);
  lcd_write_command(0xF0); lcd_write_data(0x96);
  lcd_write_command(ST7796S_MADCTL); lcd_write_data(0x48);
  lcd_write_command(ST7796S_COLMOD); lcd_write_data(0x05);
  lcd_write_command(0xB4); lcd_write_data(0x01);
  lcd_write_command(0xB7); lcd_write_data(0xC6);
  lcd_write_command(0xE8); lcd_write_data(0x40); lcd_write_data(0x8A); lcd_write_data(0x00); lcd_write_data(0x00); lcd_write_data(0x29); lcd_write_data(0x19); lcd_write_data(0xA5); lcd_write_data(0x33);
  lcd_write_command(0xC5); lcd_write_data(0x16);
  lcd_write_command(0xE0); lcd_write_data(0xF0); lcd_write_data(0x09); lcd_write_data(0x0B); lcd_write_data(0x06); lcd_write_data(0x04); lcd_write_data(0x15); lcd_write_data(0x2F); lcd_write_data(0x54); lcd_write_data(0x42); lcd_write_data(0x3C); lcd_write_data(0x17); lcd_write_data(0x14); lcd_write_data(0x18); lcd_write_data(0x1B);
  lcd_write_command(0xE1); lcd_write_data(0xF0); lcd_write_data(0x09); lcd_write_data(0x0B); lcd_write_data(0x06); lcd_write_data(0x04); lcd_write_data(0x03); lcd_write_data(0x2D); lcd_write_data(0x43); lcd_write_data(0x42); lcd_write_data(0x3B); lcd_write_data(0x16); lcd_write_data(0x14); lcd_write_data(0x17); lcd_write_data(0x1B);
  lcd_write_command(0xF0); lcd_write_data(0x3C);
  lcd_write_command(0xF0); lcd_write_data(0x69);
  lcd_write_command(ST7796S_SLPOUT); sleep_ms(120);
  lcd_write_command(ST7796S_INVON);
  lcd_write_command(ST7796S_DISPON); sleep_ms(20);
  
  printf("Display initialized (ST7796S)\r\n");
}

static void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  uint8_t cmd, data[4];

  cmd = ST7796S_CASET;
  data[0] = x0 >> 8; data[1] = x0 & 0xFF;
  data[2] = x1 >> 8; data[3] = x1 & 0xFF;
  gpio_put(LCD_CS_PIN, 0);
  gpio_put(LCD_DC_PIN, 0);
  spi_write_blocking(spi0, &cmd, 1);
  gpio_put(LCD_DC_PIN, 1);
  spi_write_blocking(spi0, data, 4);
  gpio_put(LCD_CS_PIN, 1);
  busy_wait_us(1); // Crucial minimal delay for CS high pulse

  cmd = ST7796S_RASET;
  data[0] = y0 >> 8; data[1] = y0 & 0xFF;
  data[2] = y1 >> 8; data[3] = y1 & 0xFF;
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

static void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  if (w == 0 || h == 0) return;
  if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
  if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
  if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
  display_set_window(x, y, x + w - 1, y + h - 1);
  
  uint8_t color_buf[2] = {color >> 8, color & 0xFF};
  gpio_put(LCD_DC_PIN, 1);
  gpio_put(LCD_CS_PIN, 0);
  for (uint32_t i = 0; i < (uint32_t)w * (uint32_t)h; i++) {
      spi_write_blocking(spi0, color_buf, 2);
  }
  gpio_put(LCD_CS_PIN, 1);
  busy_wait_us(1);
}

static void display_fill_screen(uint16_t color) {
  display_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

static const uint8_t *get_glyph_5x7(char c) {
  static const uint8_t glyph_space[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  static const uint8_t glyph_A[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
  static const uint8_t glyph_B[7] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
  static const uint8_t glyph_C[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
  static const uint8_t glyph_D[7] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
  static const uint8_t glyph_E[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
  static const uint8_t glyph_G[7] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E};
  static const uint8_t glyph_H[7] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
  static const uint8_t glyph_I[7] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
  static const uint8_t glyph_L_upper[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
  static const uint8_t glyph_M[7] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
  static const uint8_t glyph_N[7] = {0x11, 0x19, 0x19, 0x15, 0x13, 0x13, 0x11};
  static const uint8_t glyph_O[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
  static const uint8_t glyph_P[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
  static const uint8_t glyph_R[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
  static const uint8_t glyph_S[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
  static const uint8_t glyph_T[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
  static const uint8_t glyph_U[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
  static const uint8_t glyph_V[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
  static const uint8_t glyph_a[7] = {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F};
  static const uint8_t glyph_c[7] = {0x00, 0x00, 0x0E, 0x10, 0x10, 0x10, 0x0E};
  static const uint8_t glyph_d[7] = {0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F};
  static const uint8_t glyph_e[7] = {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E};
  static const uint8_t glyph_g[7] = {0x00, 0x00, 0x0F, 0x11, 0x0F, 0x01, 0x0E};
  static const uint8_t glyph_p[7] = {0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10};
  static const uint8_t glyph_r[7] = {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10};
  static const uint8_t glyph_i[7] = {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E};
  static const uint8_t glyph_l[7] = {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
  static const uint8_t glyph_n[7] = {0x00, 0x00, 0x1A, 0x15, 0x11, 0x11, 0x11};
  static const uint8_t glyph_K_upper[7] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
  static const uint8_t glyph_k[7] = {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12};
  static const uint8_t glyph_o[7] = {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E};
  static const uint8_t glyph_t[7] = {0x08, 0x08, 0x1C, 0x08, 0x08, 0x09, 0x06};
  static const uint8_t glyph_u[7] = {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D};
  static const uint8_t glyph_colon[7] = {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00};
  static const uint8_t glyph_0[7] = {0x0E, 0x13, 0x15, 0x15, 0x19, 0x11, 0x0E};
  static const uint8_t glyph_1[7] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
  static const uint8_t glyph_2[7] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
  static const uint8_t glyph_3[7] = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
  static const uint8_t glyph_4[7] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
  static const uint8_t glyph_6[7] = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
  static const uint8_t glyph_8[7] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};

  switch (c) {
    case 'A': return glyph_A;
    case 'B': return glyph_B;
    case 'C': return glyph_C;
    case 'D': return glyph_D;
    case 'E': return glyph_E;
    case 'G': return glyph_G;
    case 'H': return glyph_H;
    case 'I': return glyph_I;
    case 'K': return glyph_K_upper;
    case 'L': return glyph_L_upper;
    case 'M': return glyph_M;
    case 'N': return glyph_N;
    case 'O': return glyph_O;
    case 'P': return glyph_P;
    case 'R': return glyph_R;
    case 'S': return glyph_S;
    case 'T': return glyph_T;
    case 'U': return glyph_U;
    case 'V': return glyph_V;
    case 'a': return glyph_a;
    case 'c': return glyph_c;
    case 'd': return glyph_d;
    case 'e': return glyph_e;
    case 'g': return glyph_g;
    case 'r': return glyph_r;
    case 'i': return glyph_i;
    case 'l': return glyph_l;
    case 'n': return glyph_n;
    case 'k': return glyph_k;
    case 'o': return glyph_o;
    case 'p': return glyph_p;
    case 't': return glyph_t;
    case 'u': return glyph_u;
    case ':': return glyph_colon;
    case '0': return glyph_0;
    case '1': return glyph_1;
    case '2': return glyph_2;
    case '3': return glyph_3;
    case '4': return glyph_4;
    case '6': return glyph_6;
    case '8': return glyph_8;
    case ' ': return glyph_space;
    default: return glyph_space;
  }
}

static void display_draw_char_5x7(uint16_t x, uint16_t y, char c, uint16_t color, uint8_t scale) {
  const uint8_t *glyph = get_glyph_5x7(c);

  for (uint8_t row = 0; row < 7; row++) {
    for (uint8_t col = 0; col < 5; col++) {
      if (glyph[row] & (1u << (4 - col))) {
        display_fill_rect(x + (col * scale), y + (row * scale), scale, scale, color);
      }
    }
  }
}

static void display_draw_text_5x7(uint16_t x, uint16_t y, const char *text, uint16_t color, uint8_t scale) {
  while (*text) {
    display_draw_char_5x7(x, y, *text, color, scale);
    x += 6 * scale;
    text++;
  }
}

static void draw_status_bar(const char *selected_name) {
  const uint8_t text_scale = 2;
  const uint16_t text_x = 12;
  const uint16_t text_y = 16;

  display_fill_rect(0, 0, LCD_WIDTH, 60, COLOR_LIGHTGRAY);
  display_draw_text_5x7(text_x, text_y, "Selected:", COLOR_BLACK, text_scale);
  display_draw_text_5x7(text_x + (10 * 6 * text_scale), text_y, selected_name, COLOR_BLACK, text_scale);
}

typedef enum {
  INGREDIENT_GIN = 0,
  INGREDIENT_TONIC,
  INGREDIENT_CAMPARI,
  INGREDIENT_SWEET_VERMOUTH,
  INGREDIENT_APEROL,
  INGREDIENT_PROSECCO,
  INGREDIENT_SODA,
  INGREDIENT_LIMONCELLO,
  NUM_INGREDIENTS
} IngredientId;

typedef enum {
  PUMP_ID_NONE = -1,
  PUMP_ID_1 = 0,
  PUMP_ID_2,
  PUMP_ID_3,
  PUMP_ID_4,
  PUMP_ID_5,
  PUMP_ID_6,
  PUMP_ID_7,
  NUM_PUMPS
} PumpId;

typedef enum {
  SCREEN_MAIN_MENU = 0,
  SCREEN_DRINK_MENU,
  SCREEN_SHOT_MENU,
  SCREEN_SETTINGS_MENU,
  SCREEN_DRINK_DETAIL,
  SCREEN_POURING
} ScreenId;

typedef enum {
  MACHINE_IDLE = 0,
  MACHINE_DRINK_SELECTED,
  MACHINE_ATTENTION_REQUIRED
} MachineState;

typedef enum {
  SETTINGS_VIEW_PUMP_LIST = 0,
  SETTINGS_VIEW_MAPPING,
  SETTINGS_VIEW_CALIBRATION
} SettingsView;

typedef enum {
  SETTINGS_ITEM_PUMP = 0,
  SETTINGS_ITEM_CALIBRATION,
  SETTINGS_ITEM_BACK
} SettingsItemType;

typedef struct {
  const char *name;
  bool is_alcoholic;
  PumpId mapped_pump_id;
  bool enabled;
} Ingredient;

typedef struct {
  PumpId pump_id;
  IngredientId mapped_ingredient;
  bool calibration_valid;
  uint32_t calibration_ms_per_ml;
  bool enabled;
} Pump;

typedef struct {
  const char *name;
  IngredientId ingredients[4];
  uint8_t ingredient_count;
  IngredientId main_spirit;
  uint16_t default_main_spirit_ml;
} Drink;

typedef struct {
  SettingsItemType type;
  PumpId pump_id;
  const char *label;
} SettingsItem;

typedef struct {
  ScreenId current_screen;
  const Drink *selected_drink;
  IngredientId selected_shot;
  bool selected_is_shot;
  uint16_t selected_main_ml;
  MachineState machine_state;
  uint32_t pour_started_ms;
  uint32_t pour_duration_ms;
  uint16_t last_pour_progress_px;
  bool pour_complete_logged;
  PumpId selected_settings_pump;
  uint8_t drink_menu_scroll_index;
  uint8_t shot_menu_scroll_index;
  uint8_t settings_pump_scroll_index;
  uint16_t settings_pump_scroll_offset_px;
  bool settings_pump_drag_active;
  uint8_t settings_ingredient_scroll_index;
  SettingsView settings_view;
  uint32_t settings_calibration_trial_ms_per_ml;
  bool settings_calibration_saved;
} AppState;

static const uint32_t seconds_per_ml_x1000 = 1200;

static Ingredient ingredients[NUM_INGREDIENTS] = {
  {"Gin", true, PUMP_ID_1, true},
  {"Tonic", false, PUMP_ID_2, true},
  {"Campari", true, PUMP_ID_3, true},
  {"Sweet Vermouth", true, PUMP_ID_4, true},
  {"Aperol", true, PUMP_ID_5, true},
  {"Prosecco", true, PUMP_ID_6, true},
  {"Soda", false, PUMP_ID_7, true},
  {"Limoncello", true, PUMP_ID_NONE, true}
};

static Pump pumps[NUM_PUMPS] = {
  {PUMP_ID_1, INGREDIENT_GIN, true, 1200, true},
  {PUMP_ID_2, INGREDIENT_TONIC, true, 1200, true},
  {PUMP_ID_3, INGREDIENT_CAMPARI, true, 1200, true},
  {PUMP_ID_4, INGREDIENT_SWEET_VERMOUTH, true, 1200, true},
  {PUMP_ID_5, INGREDIENT_APEROL, true, 1200, true},
  {PUMP_ID_6, INGREDIENT_PROSECCO, true, 1200, true},
  {PUMP_ID_7, INGREDIENT_SODA, false, 1200, true}
};

static const Drink drinks[] = {
  {"GT", {INGREDIENT_GIN, INGREDIENT_TONIC}, 2, INGREDIENT_GIN, 50},
  {"Negroni", {INGREDIENT_GIN, INGREDIENT_CAMPARI, INGREDIENT_SWEET_VERMOUTH}, 3, INGREDIENT_GIN, 30},
  {"Aperol", {INGREDIENT_APEROL, INGREDIENT_PROSECCO, INGREDIENT_SODA}, 3, INGREDIENT_APEROL, 60},
  {"Limoncello", {INGREDIENT_LIMONCELLO, INGREDIENT_PROSECCO, INGREDIENT_SODA}, 3, INGREDIENT_LIMONCELLO, 60}
};

#define NUM_DRINKS (sizeof(drinks) / sizeof(drinks[0]))
#define MAX_DRINK_MENU_ITEMS NUM_DRINKS

static const Pump *find_pump(PumpId pump_id) {
  if (pump_id < 0 || pump_id >= NUM_PUMPS) return NULL;
  return &pumps[pump_id];
}

static bool ingredient_availability_reason(IngredientId ingredient_id,
                                           const char **reason_type,
                                           const char **reason_value,
                                           PumpId *reason_pump_id) {
  if (ingredient_id < 0 || ingredient_id >= NUM_INGREDIENTS) return false;

  const Ingredient *ingredient = &ingredients[ingredient_id];
  if (!ingredient->enabled) {
    if (reason_type != NULL) *reason_type = "missing ingredient";
    if (reason_value != NULL) *reason_value = ingredient->name;
    if (reason_pump_id != NULL) *reason_pump_id = PUMP_ID_NONE;
    return false;
  }
  if (ingredient->mapped_pump_id == PUMP_ID_NONE) {
    if (reason_type != NULL) *reason_type = "missing pump mapping";
    if (reason_value != NULL) *reason_value = ingredient->name;
    if (reason_pump_id != NULL) *reason_pump_id = PUMP_ID_NONE;
    return false;
  }

  const Pump *pump = find_pump(ingredient->mapped_pump_id);
  if (pump == NULL) {
    if (reason_type != NULL) *reason_type = "missing pump mapping";
    if (reason_value != NULL) *reason_value = ingredient->name;
    if (reason_pump_id != NULL) *reason_pump_id = ingredient->mapped_pump_id;
    return false;
  }
  if (!pump->enabled) {
    if (reason_type != NULL) *reason_type = "pump disabled";
    if (reason_value != NULL) *reason_value = NULL;
    if (reason_pump_id != NULL) *reason_pump_id = pump->pump_id;
    return false;
  }
  if (pump->mapped_ingredient != ingredient_id) {
    if (reason_type != NULL) *reason_type = "mapping mismatch";
    if (reason_value != NULL) *reason_value = ingredient->name;
    if (reason_pump_id != NULL) *reason_pump_id = pump->pump_id;
    return false;
  }

  return true;
}

static bool ingredient_is_available(IngredientId ingredient_id) {
  return ingredient_availability_reason(ingredient_id, NULL, NULL, NULL);
}

static void log_availability_report(const char *label,
                                    bool available,
                                    const char *reason_type,
                                    const char *reason_value,
                                    PumpId reason_pump_id) {
  if (available) {
    printf("%s available=yes (shown)\r\n", label);
  } else if (reason_type != NULL && strcmp(reason_type, "pump disabled") == 0 && reason_pump_id != PUMP_ID_NONE) {
    printf("%s available=no (hidden; %s: pump %d)\r\n",
           label,
           reason_type,
           (int)reason_pump_id + 1);
  } else if (reason_type != NULL && reason_value != NULL) {
    printf("%s available=no (hidden; %s: %s)\r\n",
           label,
           reason_type,
           reason_value);
  } else if (reason_type != NULL) {
    printf("%s available=no (hidden; %s)\r\n", label, reason_type);
  } else {
    printf("%s available=no (hidden)\r\n", label);
  }
}

static bool drink_is_available(const Drink *drink) {
  if (drink == NULL) return false;

  for (uint8_t i = 0; i < drink->ingredient_count; i++) {
    if (!ingredient_is_available(drink->ingredients[i])) {
      return false;
    }
  }

  return true;
}

static uint32_t calculate_pour_time_ms(uint16_t ml) {
  return (uint32_t)ml * seconds_per_ml_x1000;
}

static uint16_t normalize_main_spirit_ml(uint16_t ml) {
  if (ml <= 50) return 40;
  if (ml <= 70) return 60;
  return 80;
}

static bool ingredient_is_available(IngredientId ingredient_id);
static bool shot_availability_reason(IngredientId ingredient_id,
                                     const char **reason_type,
                                     const char **reason_value,
                                     PumpId *reason_pump_id);

static const char *get_shot_label(IngredientId ingredient_id) {
  switch (ingredient_id) {
    case INGREDIENT_GIN: return "GIN";
    case INGREDIENT_CAMPARI: return "CAMPARI";
    case INGREDIENT_SWEET_VERMOUTH: return "VERMOUTH";
    case INGREDIENT_APEROL: return "APEROL";
    case INGREDIENT_PROSECCO: return "PROSECCO";
    case INGREDIENT_LIMONCELLO: return "LIMONCELLO";
    default: return "SHOT";
  }
}

static const char *get_selected_label(const AppState *app_state) {
  if (app_state->selected_is_shot) {
    return get_shot_label(app_state->selected_shot);
  }
  if (app_state->selected_drink != NULL) {
    return app_state->selected_drink->name;
  }
  return "none";
}

#define MAX_SHOT_MENU_ITEMS 5

static size_t collect_available_shots(IngredientId *out, size_t max_items) {
  size_t count = 0;

  for (int i = 0; i < NUM_INGREDIENTS && count < max_items; i++) {
    if (!ingredients[i].is_alcoholic) continue;
    if (!shot_availability_reason((IngredientId)i, NULL, NULL, NULL)) continue;
    out[count++] = (IngredientId)i;
  }

  return count;
}

static bool drink_availability_reason(const Drink *drink,
                                      const char **reason_type,
                                      const char **reason_value,
                                      PumpId *reason_pump_id) {
  if (drink == NULL) return false;

  for (uint8_t i = 0; i < drink->ingredient_count; i++) {
    if (!ingredient_availability_reason(drink->ingredients[i], reason_type, reason_value, reason_pump_id)) {
      return false;
    }
  }

  return true;
}

static bool shot_availability_reason(IngredientId ingredient_id,
                                     const char **reason_type,
                                     const char **reason_value,
                                     PumpId *reason_pump_id) {
  if (ingredient_id < 0 || ingredient_id >= NUM_INGREDIENTS) return false;
  if (!ingredients[ingredient_id].is_alcoholic) {
    if (reason_type != NULL) *reason_type = "not alcoholic";
    if (reason_value != NULL) *reason_value = ingredients[ingredient_id].name;
    if (reason_pump_id != NULL) *reason_pump_id = PUMP_ID_NONE;
    return false;
  }

  return ingredient_availability_reason(ingredient_id, reason_type, reason_value, reason_pump_id);
}

static const char *get_ingredient_name(IngredientId ingredient_id) {
  if (ingredient_id < 0 || ingredient_id >= NUM_INGREDIENTS) return "NONE";
  return ingredients[ingredient_id].name;
}

static const char *get_pump_mapping_name(PumpId pump_id) {
  const Pump *pump = find_pump(pump_id);
  if (pump == NULL) return "NONE";
  return get_ingredient_name(pump->mapped_ingredient);
}

static uint32_t get_pump_calibration_ms_per_ml(PumpId pump_id) {
  const Pump *pump = find_pump(pump_id);
  if (pump == NULL) return seconds_per_ml_x1000;
  if (!pump->calibration_valid) return seconds_per_ml_x1000;
  return pump->calibration_ms_per_ml;
}

static void apply_pump_mapping(PumpId pump_id, IngredientId ingredient_id) {
  if (pump_id < 0 || pump_id >= NUM_PUMPS) return;
  if (ingredient_id < 0 || ingredient_id >= NUM_INGREDIENTS) return;

  Pump *pump = &pumps[pump_id];
  const IngredientId old_ingredient_id = pump->mapped_ingredient;
  const PumpId other_pump_id = ingredients[ingredient_id].mapped_pump_id;

  if (old_ingredient_id >= 0 && old_ingredient_id < NUM_INGREDIENTS) {
    ingredients[old_ingredient_id].mapped_pump_id = PUMP_ID_NONE;
  }

  if (other_pump_id >= 0 && other_pump_id < NUM_PUMPS && other_pump_id != pump_id) {
    pumps[other_pump_id].mapped_ingredient = old_ingredient_id;
    if (old_ingredient_id >= 0 && old_ingredient_id < NUM_INGREDIENTS) {
      ingredients[old_ingredient_id].mapped_pump_id = other_pump_id;
    }
  }

  pump->mapped_ingredient = ingredient_id;
  ingredients[ingredient_id].mapped_pump_id = pump_id;

  printf("Pump mapping updated: Pump %d -> %s\r\n",
         (int)pump_id + 1,
         get_ingredient_name(ingredient_id));
  if (other_pump_id >= 0 && other_pump_id < NUM_PUMPS && other_pump_id != pump_id) {
    printf("Pump mapping swap: Pump %d -> %s\r\n",
           (int)other_pump_id + 1,
           get_ingredient_name(old_ingredient_id));
  }
  fflush(stdout);
}

static size_t collect_available_drinks(const Drink **out, size_t max_items) {
  size_t count = 0;

  for (size_t i = 0; i < NUM_DRINKS && count < max_items; i++) {
    if (!drink_availability_reason(&drinks[i], NULL, NULL, NULL)) continue;
    out[count++] = &drinks[i];
  }

  return count;
}

static void draw_drink_menu_zone(const AppState *app_state, int idx, bool active);
static void draw_shot_menu_zone(const AppState *app_state, int idx, bool active);

typedef struct {
    uint16_t x, y, w, h;
    uint16_t color;
    const char *name;
} Zone;

#define NUM_MAIN_MENU_ZONES 3
static const Zone main_menu_zones[NUM_MAIN_MENU_ZONES] = {
    {20, 80,  280, 100, COLOR_BLUE, "DRINKS"},
    {20, 200, 280, 100, COLOR_GREEN, "SHOTS"},
    {20, 320, 280, 100, COLOR_LIGHTGRAY, "SETTINGS"}
};

#define NUM_DRINK_MENU_ZONES 5
#define DRINK_MENU_VISIBLE_ITEMS (NUM_DRINK_MENU_ZONES - 1)
static const Zone drink_menu_zones[NUM_DRINK_MENU_ZONES] = {
    {20, 70,  280, 55, COLOR_BLUE, ""},
    {20, 130, 280, 55, COLOR_BLUE, ""},
    {20, 190, 280, 55, COLOR_BLUE, ""},
    {20, 250, 280, 55, COLOR_BLUE, ""},
    {20, 380, 280, 55, COLOR_LIGHTGRAY, "BACK"}
};

#define NUM_SHOT_MENU_ZONES 6
#define SHOT_MENU_VISIBLE_ITEMS (NUM_SHOT_MENU_ZONES - 1)
static const Zone shot_menu_zones[NUM_SHOT_MENU_ZONES] = {
    {20, 70,  280, 55, COLOR_GREEN, ""},
    {20, 130, 280, 55, COLOR_GREEN, ""},
    {20, 190, 280, 55, COLOR_GREEN, ""},
    {20, 250, 280, 55, COLOR_GREEN, ""},
    {20, 310, 280, 55, COLOR_GREEN, ""},
    {20, 380, 280, 55, COLOR_LIGHTGRAY, "BACK"}
};

static const Zone menu_back_zone = {20, 370, 280, 70, COLOR_LIGHTGRAY, "BACK"};

#define SETTINGS_VISIBLE_ITEMS 5
#define SETTINGS_INGREDIENT_VISIBLE_ITEMS 4
#define SETTINGS_MENU_PUMP_COUNT 6
#define SETTINGS_MAIN_ITEM_COUNT (SETTINGS_MENU_PUMP_COUNT + 2)
#define NUM_SETTINGS_PUMP_ZONES SETTINGS_VISIBLE_ITEMS
#define SETTINGS_PUMP_ROW_HEIGHT 60
#define SETTINGS_INGREDIENT_ZONE_BACK 4
#define NUM_SETTINGS_INGREDIENT_ZONES 5
#define NUM_SETTINGS_CALIBRATION_ZONES 5

static const Zone settings_pump_zones[NUM_SETTINGS_PUMP_ZONES] = {
    {20, 70,  280, 55, COLOR_BLUE, ""},
    {20, 130, 280, 55, COLOR_BLUE, ""},
    {20, 190, 280, 55, COLOR_BLUE, ""},
    {20, 250, 280, 55, COLOR_BLUE, ""},
    {20, 310, 280, 55, COLOR_BLUE, ""}
};

static const Zone settings_ingredient_zones[NUM_SETTINGS_INGREDIENT_ZONES] = {
    {20, 90,  280, 45, COLOR_GREEN, ""},
    {20, 140, 280, 45, COLOR_GREEN, ""},
    {20, 190, 280, 45, COLOR_GREEN, ""},
    {20, 240, 280, 45, COLOR_GREEN, ""},
    {215,310, 85,  55, COLOR_LIGHTGRAY, "BACK"}
};

static const Zone settings_calibration_zones[NUM_SETTINGS_CALIBRATION_ZONES] = {
    {20,  300, 70,  45, COLOR_LIGHTGRAY, "PREV"},
    {100, 300, 70,  45, COLOR_LIGHTGRAY, "NEXT"},
    {20,  360, 90,  55, COLOR_GREEN,     "TEST"},
    {125, 360, 90,  55, COLOR_BLUE,      "SAVE"},
    {230, 360, 70,  55, COLOR_LIGHTGRAY, "BACK"}
};

#define TOUCH_SCROLL_THRESHOLD_PX 18

typedef enum {
    DETAIL_ZONE_ML_40 = 0,
    DETAIL_ZONE_ML_60,
    DETAIL_ZONE_ML_80,
    DETAIL_ZONE_POUR,
    DETAIL_ZONE_BACK,
    NUM_DETAIL_ZONES
} DetailZoneId;

static const Zone detail_zones[NUM_DETAIL_ZONES] = {
    {20, 150, 80, 70, COLOR_BLUE, "40"},
    {120, 150, 80, 70, COLOR_BLUE, "60"},
    {220, 150, 80, 70, COLOR_BLUE, "80"},
    {20, 260, 180, 80, COLOR_GREEN, "POUR"},
    {220, 260, 80, 80, COLOR_LIGHTGRAY, "BACK"}
};

static bool point_in_zone(uint16_t x, uint16_t y, const Zone *z) {
    return x >= z->x && x < z->x + z->w && y >= z->y && y < z->y + z->h;
}

static void draw_labeled_zone(const Zone *z, uint16_t fill_color, uint16_t text_color, uint8_t text_scale) {
    display_fill_rect(z->x, z->y, z->w, z->h, fill_color);

    const size_t label_len = strlen(z->name);
    const uint16_t text_width = (uint16_t)(label_len * 6 * text_scale);
    const uint16_t text_height = 7 * text_scale;
    const uint16_t text_x = z->x + ((z->w - text_width) / 2);
    const uint16_t text_y = z->y + ((z->h - text_height) / 2);
    display_draw_text_5x7(text_x, text_y, z->name, text_color, text_scale);
}

// Render a specific zone box, either normal color or white highlight
static void draw_zone_from_array(const Zone *zones, int count, int idx, bool active) {
    if (idx < 0 || idx >= count) return;
    const Zone *z = &zones[idx];
    draw_labeled_zone(z, active ? COLOR_WHITE : z->color, active ? COLOR_BLUE : COLOR_WHITE, 4);
}

static void draw_main_menu_screen(void) {
  printf("Drawing main menu...\r\n");
  display_fill_screen(COLOR_DARKGRAY);
  draw_status_bar("MAIN");
  
  for (int i = 0; i < NUM_MAIN_MENU_ZONES; i++) {
      draw_zone_from_array(main_menu_zones, NUM_MAIN_MENU_ZONES, i, false);
  }
  printf("Main menu drawn.\r\n");
}

static void draw_drink_menu_screen(const AppState *app_state) {
  printf("Drawing drink menu...\r\n");
  display_fill_screen(COLOR_DARKGRAY);
  draw_status_bar("DRINKS");

  for (size_t i = 0; i < NUM_DRINKS; i++) {
    const char *reason_type = NULL;
    const char *reason_value = NULL;
    PumpId reason_pump_id = PUMP_ID_NONE;
    const bool available = drink_availability_reason(&drinks[i], &reason_type, &reason_value, &reason_pump_id);

    log_availability_report(drinks[i].name, available, reason_type, reason_value, reason_pump_id);
  }
  fflush(stdout);

  for (int i = 0; i < NUM_DRINK_MENU_ZONES; i++) {
      draw_drink_menu_zone(app_state, i, false);
  }
  printf("Drink menu drawn.\r\n");
}

static void draw_drink_menu_zone(const AppState *app_state, int idx, bool active) {
  if (idx < 0 || idx >= NUM_DRINK_MENU_ZONES) return;

  const Drink *available_drinks[MAX_DRINK_MENU_ITEMS];
  const size_t drink_count = collect_available_drinks(available_drinks, MAX_DRINK_MENU_ITEMS);
  const Zone *z = &drink_menu_zones[idx];
  const size_t visible_index = (size_t)idx + app_state->drink_menu_scroll_index;

  if (idx < DRINK_MENU_VISIBLE_ITEMS && visible_index < drink_count) {
    const char *label = available_drinks[visible_index]->name;
    draw_labeled_zone(z, active ? COLOR_WHITE : z->color, active ? COLOR_BLUE : COLOR_WHITE, 3);

    const uint16_t text_y = z->y + ((z->h - (7 * 3)) / 2);
    const uint16_t text_x = z->x + ((z->w - ((uint16_t)strlen(label) * 18)) / 2);
    display_draw_text_5x7(text_x, text_y, label, active ? COLOR_BLUE : COLOR_WHITE, 3);
  } else if (idx == NUM_DRINK_MENU_ZONES - 1) {
    draw_labeled_zone(z, active ? COLOR_WHITE : z->color, active ? COLOR_BLUE : COLOR_BLACK, 4);
  } else {
    display_fill_rect(z->x, z->y, z->w, z->h, COLOR_DARKGRAY);
  }
}

static void draw_shot_menu_zone(const AppState *app_state, int idx, bool active) {
  if (idx < 0 || idx >= NUM_SHOT_MENU_ZONES) return;

  IngredientId available_shots[MAX_SHOT_MENU_ITEMS];
  const size_t shot_count = collect_available_shots(available_shots, MAX_SHOT_MENU_ITEMS);
  const Zone *z = &shot_menu_zones[idx];
  const size_t visible_index = (size_t)idx + app_state->shot_menu_scroll_index;

  if (idx < SHOT_MENU_VISIBLE_ITEMS && visible_index < shot_count) {
    const char *label = get_shot_label(available_shots[visible_index]);
    draw_labeled_zone(z, active ? COLOR_WHITE : z->color, active ? COLOR_BLUE : COLOR_WHITE, 3);

    const uint16_t text_y = z->y + ((z->h - (7 * 3)) / 2);
    const uint16_t text_x = z->x + ((z->w - ((uint16_t)strlen(label) * 18)) / 2);
    display_draw_text_5x7(text_x, text_y, label, active ? COLOR_BLUE : COLOR_WHITE, 3);
  } else if (idx == NUM_SHOT_MENU_ZONES - 1) {
    draw_labeled_zone(z, active ? COLOR_WHITE : z->color, active ? COLOR_BLUE : COLOR_BLACK, 4);
  } else {
    display_fill_rect(z->x, z->y, z->w, z->h, COLOR_DARKGRAY);
  }
}

static void draw_shot_menu_screen(const AppState *app_state) {
  printf("Drawing shot menu...\r\n");
  display_fill_screen(COLOR_DARKGRAY);
  draw_status_bar("SHOTS");

  for (int i = 0; i < NUM_INGREDIENTS; i++) {
    const IngredientId shot = (IngredientId)i;
    if (!ingredients[i].is_alcoholic) continue;

    const char *reason_type = NULL;
    const char *reason_value = NULL;
    PumpId reason_pump_id = PUMP_ID_NONE;
    const bool available = shot_availability_reason(shot, &reason_type, &reason_value, &reason_pump_id);

    log_availability_report(get_shot_label(shot), available, reason_type, reason_value, reason_pump_id);
  }
  fflush(stdout);

  for (int i = 0; i < NUM_SHOT_MENU_ZONES; i++) {
    draw_shot_menu_zone(app_state, i, false);
  }
  printf("Shot menu drawn.\r\n");
}

static uint8_t get_scroll_max(size_t item_count, uint8_t visible_count) {
  if (item_count <= visible_count) return 0;
  return (uint8_t)(item_count - visible_count);
}

static size_t build_settings_items(SettingsItem *out, size_t max_items) {
  size_t count = 0;
  static char labels[SETTINGS_MENU_PUMP_COUNT][32];

  printf("Building settings items...\r\n");

  for (int i = 0; i < SETTINGS_MENU_PUMP_COUNT && count < max_items; i++) {
    const char *mapping_name = get_pump_mapping_name((PumpId)i);
    snprintf(labels[i], sizeof(labels[i]), "P%d -> %s", i + 1, mapping_name);
    out[count].type = SETTINGS_ITEM_PUMP;
    out[count].pump_id = (PumpId)i;
    out[count].label = labels[i];
    printf("Settings item: Pump %d -> %s\r\n", i + 1, mapping_name);
    count++;
  }

  if (count < max_items) {
    out[count].type = SETTINGS_ITEM_CALIBRATION;
    out[count].pump_id = PUMP_ID_NONE;
    out[count].label = "Calibration";
    printf("Settings item: Calibration\r\n");
    count++;
  }

  if (count < max_items) {
    out[count].type = SETTINGS_ITEM_BACK;
    out[count].pump_id = PUMP_ID_NONE;
    out[count].label = "Back";
    printf("Settings item: Back\r\n");
    count++;
  }

  printf("Settings item count=%u\r\n", (unsigned int)count);

  return count;
}

static uint8_t get_settings_pump_scroll_max(void) {
  SettingsItem items[SETTINGS_MAIN_ITEM_COUNT];
  const size_t item_count = build_settings_items(items, SETTINGS_MAIN_ITEM_COUNT);
  return get_scroll_max(item_count, SETTINGS_VISIBLE_ITEMS);
}

static uint16_t get_settings_pump_viewport_height(void) {
  const Zone *first = &settings_pump_zones[0];
  const Zone *last = &settings_pump_zones[NUM_SETTINGS_PUMP_ZONES - 1];
  return (uint16_t)((last->y + last->h) - first->y);
}

static uint16_t get_settings_pump_scroll_offset_max(void) {
  SettingsItem items[SETTINGS_MAIN_ITEM_COUNT];
  const size_t item_count = build_settings_items(items, SETTINGS_MAIN_ITEM_COUNT);
  const uint32_t content_height = (uint32_t)item_count * SETTINGS_PUMP_ROW_HEIGHT;
  const uint16_t viewport_height = get_settings_pump_viewport_height();

  if (content_height <= viewport_height) return 0;
  return (uint16_t)(content_height - viewport_height);
}

static uint16_t clamp_settings_pump_scroll_offset(uint16_t offset_px) {
  const uint16_t max_offset = get_settings_pump_scroll_offset_max();
  return offset_px > max_offset ? max_offset : offset_px;
}

static bool scroll_settings_pump_by_pixels(AppState *app_state, int16_t delta_y) {
  if (delta_y == 0) return false;

  const uint16_t old_offset = app_state->settings_pump_scroll_offset_px;
  const int32_t unclamped_offset = (int32_t)old_offset - (int32_t)delta_y;
  uint16_t new_offset = 0;

  if (unclamped_offset > 0) {
    new_offset = clamp_settings_pump_scroll_offset((uint16_t)unclamped_offset);
  }

  if (new_offset == old_offset) {
    printf("SETTINGS px scroll blocked old=%u new=%u delta_y=%d max=%u\r\n",
           (unsigned int)old_offset,
           (unsigned int)new_offset,
           (int)delta_y,
           (unsigned int)get_settings_pump_scroll_offset_max());
    return false;
  }

  app_state->settings_pump_scroll_offset_px = new_offset;
  printf("SETTINGS px scroll old=%u new=%u delta_y=%d max=%u\r\n",
         (unsigned int)old_offset,
         (unsigned int)new_offset,
         (int)delta_y,
         (unsigned int)get_settings_pump_scroll_offset_max());
  return true;
}

static uint8_t get_settings_ingredient_scroll_max(void) {
  return get_scroll_max(NUM_INGREDIENTS, SETTINGS_INGREDIENT_VISIBLE_ITEMS);
}

static uint8_t get_drink_menu_scroll_max(void) {
  const Drink *available_drinks[MAX_DRINK_MENU_ITEMS];
  const size_t drink_count = collect_available_drinks(available_drinks, MAX_DRINK_MENU_ITEMS);
  return get_scroll_max(drink_count, DRINK_MENU_VISIBLE_ITEMS);
}

static uint8_t get_shot_menu_scroll_max(void) {
  IngredientId available_shots[MAX_SHOT_MENU_ITEMS];
  const size_t shot_count = collect_available_shots(available_shots, MAX_SHOT_MENU_ITEMS);
  return get_scroll_max(shot_count, SHOT_MENU_VISIBLE_ITEMS);
}

static bool point_in_drink_list(uint16_t x, uint16_t y) {
  for (int i = 0; i < DRINK_MENU_VISIBLE_ITEMS; i++) {
    if (point_in_zone(x, y, &drink_menu_zones[i])) return true;
  }
  return false;
}

static bool point_in_shot_list(uint16_t x, uint16_t y) {
  for (int i = 0; i < SHOT_MENU_VISIBLE_ITEMS; i++) {
    if (point_in_zone(x, y, &shot_menu_zones[i])) return true;
  }
  return false;
}

static bool point_in_settings_pump_list(uint16_t x, uint16_t y) {
  const Zone *first = &settings_pump_zones[0];
  const Zone *last = &settings_pump_zones[SETTINGS_VISIBLE_ITEMS - 1];
  const uint16_t list_x0 = first->x;
  const uint16_t list_y0 = first->y;
  const uint16_t list_x1 = first->x + first->w;
  const uint16_t list_y1 = last->y + last->h;
  return x >= list_x0 && x < list_x1 && y >= list_y0 && y < list_y1;
}

static bool point_in_settings_ingredient_list(uint16_t x, uint16_t y) {
  const Zone *first = &settings_ingredient_zones[0];
  const Zone *last = &settings_ingredient_zones[SETTINGS_INGREDIENT_VISIBLE_ITEMS - 1];
  const uint16_t list_x0 = first->x;
  const uint16_t list_y0 = first->y;
  const uint16_t list_x1 = first->x + first->w;
  const uint16_t list_y1 = last->y + last->h;
  return x >= list_x0 && x < list_x1 && y >= list_y0 && y < list_y1;
}

static bool touch_targets_scrollable_list(const AppState *app_state, uint16_t x, uint16_t y) {
  if (app_state->current_screen == SCREEN_DRINK_MENU) {
    return point_in_drink_list(x, y);
  }
  if (app_state->current_screen == SCREEN_SHOT_MENU) {
    return point_in_shot_list(x, y);
  }
  if (app_state->current_screen == SCREEN_SETTINGS_MENU) {
    if (app_state->settings_view == SETTINGS_VIEW_PUMP_LIST) {
      return point_in_settings_pump_list(x, y);
    }
    if (app_state->settings_view == SETTINGS_VIEW_MAPPING) {
      return point_in_settings_ingredient_list(x, y);
    }
  }
  return false;
}

static bool apply_vertical_scroll(AppState *app_state, int16_t delta_y) {
  if (delta_y == 0) return false;

  uint8_t *scroll_index = NULL;
  uint8_t scroll_max = 0;
  const char *scroll_label = "LIST";

  if (app_state->current_screen == SCREEN_DRINK_MENU) {
    scroll_index = &app_state->drink_menu_scroll_index;
    scroll_max = get_drink_menu_scroll_max();
    scroll_label = "DRINKS";
  } else if (app_state->current_screen == SCREEN_SHOT_MENU) {
    scroll_index = &app_state->shot_menu_scroll_index;
    scroll_max = get_shot_menu_scroll_max();
    scroll_label = "SHOTS";
  } else if (app_state->current_screen == SCREEN_SETTINGS_MENU) {
    if (app_state->settings_view == SETTINGS_VIEW_PUMP_LIST) {
      scroll_index = &app_state->settings_pump_scroll_index;
      scroll_max = get_settings_pump_scroll_max();
      scroll_label = "SETTINGS";
    } else if (app_state->settings_view == SETTINGS_VIEW_MAPPING) {
      scroll_index = &app_state->settings_ingredient_scroll_index;
      scroll_max = get_settings_ingredient_scroll_max();
      scroll_label = "MAPPING";
    }
  }

  if (scroll_index == NULL) return false;
  if (scroll_max == 0) {
    printf("%s scroll blocked old=%u new=%u max=%u delta_y=%d (content fits)\r\n",
           scroll_label,
           (unsigned int)*scroll_index,
           (unsigned int)*scroll_index,
           (unsigned int)scroll_max,
           (int)delta_y);
    return false;
  }

  const uint8_t old_index = *scroll_index;
  uint8_t new_index = *scroll_index;
  if (delta_y < 0 && new_index < scroll_max) {
    new_index++;
  } else if (delta_y > 0 && new_index > 0) {
    new_index--;
  }

  if (new_index == *scroll_index) {
    printf("%s scroll blocked old=%u new=%u max=%u delta_y=%d\r\n",
           scroll_label,
           (unsigned int)old_index,
           (unsigned int)new_index,
           (unsigned int)scroll_max,
           (int)delta_y);
    return false;
  }

  *scroll_index = new_index;
  printf("%s scroll old=%u new=%u max=%u delta_y=%d\r\n",
         scroll_label,
         (unsigned int)old_index,
         (unsigned int)*scroll_index,
         (unsigned int)scroll_max,
         (int)delta_y);
  return true;
}

static void draw_settings_pump_zone(const AppState *app_state, int idx, bool active) {
  if (idx < 0 || idx >= NUM_SETTINGS_PUMP_ZONES) return;

  SettingsItem items[SETTINGS_MAIN_ITEM_COUNT];
  const size_t item_count = build_settings_items(items, SETTINGS_MAIN_ITEM_COUNT);
  const Zone *z = &settings_pump_zones[idx];
  const int item_index = app_state->settings_pump_scroll_index + idx;

  if (idx < SETTINGS_VISIBLE_ITEMS && item_index < (int)item_count) {
    const SettingsItem *item = &items[item_index];
    uint16_t fill_color = item->type == SETTINGS_ITEM_PUMP ? COLOR_BLUE : COLOR_LIGHTGRAY;
    uint16_t text_color = item->type == SETTINGS_ITEM_PUMP ? COLOR_WHITE : COLOR_BLACK;

    if (item->type == SETTINGS_ITEM_PUMP && item->pump_id == app_state->selected_settings_pump) {
      fill_color = COLOR_GREEN;
    }
    if (active) {
      fill_color = COLOR_WHITE;
      text_color = COLOR_BLUE;
    }

    printf("Visible settings item %d = global %d (%s)\r\n", idx, item_index, item->label);
    draw_labeled_zone(z, fill_color, text_color, 2);

    const uint16_t text_y = z->y + ((z->h - (7 * 2)) / 2);
    const uint16_t text_x = z->x + ((z->w - ((uint16_t)strlen(item->label) * 12)) / 2);
    display_draw_text_5x7(text_x, text_y, item->label, text_color, 2);
  } else if (idx >= SETTINGS_VISIBLE_ITEMS) {
    display_fill_rect(z->x, z->y, z->w, z->h, COLOR_DARKGRAY);
  } else {
    printf("Visible settings item %d = <empty>\r\n", idx);
    display_fill_rect(z->x, z->y, z->w, z->h, COLOR_DARKGRAY);
  }
}

static int get_settings_pump_item_index_at_point(const AppState *app_state,
                                                 uint16_t x,
                                                 uint16_t y) {
  const Zone *first = &settings_pump_zones[0];
  const uint16_t viewport_y = first->y;
  const uint16_t viewport_bottom = viewport_y + get_settings_pump_viewport_height();

  if (x < first->x || x >= first->x + first->w) return -1;
  if (y < viewport_y || y >= viewport_bottom) return -1;

  const uint32_t content_y =
      (uint32_t)app_state->settings_pump_scroll_offset_px + (uint32_t)(y - viewport_y);
  const uint32_t row_index = content_y / SETTINGS_PUMP_ROW_HEIGHT;
  const uint32_t row_local_y = content_y % SETTINGS_PUMP_ROW_HEIGHT;

  if (row_local_y >= settings_pump_zones[0].h) return -1;

  SettingsItem items[SETTINGS_MAIN_ITEM_COUNT];
  const size_t item_count = build_settings_items(items, SETTINGS_MAIN_ITEM_COUNT);
  if (row_index >= item_count) return -1;

  return (int)row_index;
}

static void draw_settings_edit_zone(const AppState *app_state, int idx, bool active) {
  if (idx < 0 || idx >= NUM_SETTINGS_INGREDIENT_ZONES) return;

  const Zone *z = &settings_ingredient_zones[idx];
  const int ingredient_start = app_state->settings_ingredient_scroll_index;
  const int ingredient_idx = ingredient_start + idx;

  if (idx < SETTINGS_INGREDIENT_VISIBLE_ITEMS && ingredient_idx < NUM_INGREDIENTS) {
    const char *label = get_ingredient_name((IngredientId)ingredient_idx);
    printf("Visible mapping item %d = %s\r\n", idx, label);
    draw_labeled_zone(z, active ? COLOR_WHITE : z->color, active ? COLOR_BLUE : COLOR_WHITE, 2);

    const uint16_t text_y = z->y + ((z->h - (7 * 2)) / 2);
    const uint16_t text_x = z->x + ((z->w - ((uint16_t)strlen(label) * 12)) / 2);
    display_draw_text_5x7(text_x, text_y, label, active ? COLOR_BLUE : COLOR_WHITE, 2);
  } else if (idx == SETTINGS_INGREDIENT_ZONE_BACK) {
    draw_labeled_zone(z, active ? COLOR_WHITE : z->color, active ? COLOR_BLUE : COLOR_BLACK, 2);
  } else if (idx >= SETTINGS_INGREDIENT_VISIBLE_ITEMS) {
    display_fill_rect(z->x, z->y, z->w, z->h, COLOR_DARKGRAY);
  } else {
    display_fill_rect(z->x, z->y, z->w, z->h, COLOR_DARKGRAY);
  }
}

static void draw_settings_calibration_zone(int idx, bool active) {
  if (idx < 0 || idx >= NUM_SETTINGS_CALIBRATION_ZONES) return;

  const Zone *z = &settings_calibration_zones[idx];
  const uint8_t text_scale = (strlen(z->name) > 4) ? 1 : 2;
  draw_labeled_zone(z, active ? COLOR_WHITE : z->color, active ? COLOR_BLUE : COLOR_BLACK, text_scale);
}

static void redraw_settings_list_area(const AppState *app_state) {
  if (app_state->settings_view == SETTINGS_VIEW_PUMP_LIST) {
    const Zone *first = &settings_pump_zones[0];
    const uint16_t viewport_y = first->y;
    const uint16_t viewport_h = get_settings_pump_viewport_height();
    const uint16_t viewport_bottom = viewport_y + viewport_h;
    const uint16_t row_h = settings_pump_zones[0].h;
    const bool simplified_drag_render = app_state->settings_pump_drag_active;
    SettingsItem items[SETTINGS_MAIN_ITEM_COUNT];
    const size_t item_count = build_settings_items(items, SETTINGS_MAIN_ITEM_COUNT);

    display_fill_rect(first->x, viewport_y, first->w, viewport_h, COLOR_DARKGRAY);

    for (size_t i = 0; i < item_count; i++) {
      const SettingsItem *item = &items[i];
      const int32_t item_y =
          (int32_t)viewport_y + ((int32_t)i * SETTINGS_PUMP_ROW_HEIGHT) -
          (int32_t)app_state->settings_pump_scroll_offset_px;
      const int32_t item_bottom = item_y + row_h;

      if (item_bottom <= viewport_y || item_y >= viewport_bottom) continue;

      uint16_t fill_color =
          item->type == SETTINGS_ITEM_PUMP ? COLOR_BLUE : COLOR_LIGHTGRAY;
      uint16_t text_color =
          item->type == SETTINGS_ITEM_PUMP ? COLOR_WHITE : COLOR_BLACK;

      if (item->type == SETTINGS_ITEM_PUMP &&
          item->pump_id == app_state->selected_settings_pump) {
        fill_color = COLOR_GREEN;
      }

      const int32_t visible_top = item_y < viewport_y ? viewport_y : item_y;
      const int32_t visible_bottom =
          item_bottom > viewport_bottom ? viewport_bottom : item_bottom;
      display_fill_rect(first->x, (uint16_t)visible_top, first->w,
                        (uint16_t)(visible_bottom - visible_top), fill_color);

      if (!simplified_drag_render &&
          item_y >= viewport_y && item_bottom <= viewport_bottom) {
        const uint16_t text_y = (uint16_t)item_y + ((row_h - (7 * 2)) / 2);
        const uint16_t text_x =
            first->x + ((first->w - ((uint16_t)strlen(item->label) * 12)) / 2);
        display_draw_text_5x7(text_x, text_y, item->label, text_color, 2);
      }
    }
  } else if (app_state->settings_view == SETTINGS_VIEW_MAPPING) {
    const Zone *first = &settings_ingredient_zones[0];
    const Zone *last =
        &settings_ingredient_zones[SETTINGS_INGREDIENT_VISIBLE_ITEMS - 1];
    display_fill_rect(first->x, first->y, first->w,
                      (last->y + last->h) - first->y, COLOR_DARKGRAY);
    for (int i = 0; i < NUM_SETTINGS_INGREDIENT_ZONES; i++) {
      draw_settings_edit_zone(app_state, i, false);
    }
  }
}

static void draw_settings_screen(const AppState *app_state) {
  display_fill_screen(COLOR_BLACK);
  display_fill_rect(0, 0, LCD_WIDTH, 70, COLOR_BLUE);

  if (app_state->settings_view == SETTINGS_VIEW_PUMP_LIST) {
    SettingsItem items[SETTINGS_MAIN_ITEM_COUNT];
    const size_t item_count = build_settings_items(items, SETTINGS_MAIN_ITEM_COUNT);
    printf("Drawing settings list from index=%u max=%u selected_pump=%d\r\n",
           (unsigned int)app_state->settings_pump_scroll_index,
           (unsigned int)get_settings_pump_scroll_max(),
           (int)app_state->selected_settings_pump + 1);
    display_draw_text_5x7(35, 18, "SETTINGS", COLOR_WHITE, 4);
    draw_status_bar("LIST");
    printf("Settings list item_count=%u visible=%u\r\n",
           (unsigned int)item_count,
           (unsigned int)SETTINGS_VISIBLE_ITEMS);
    redraw_settings_list_area(app_state);
  } else if (app_state->settings_view == SETTINGS_VIEW_MAPPING) {
    char title[32];
    snprintf(title, sizeof(title), "PUMP %d", (int)app_state->selected_settings_pump + 1);
    printf("Drawing mapping list from index=%u max=%u for pump %d\r\n",
           (unsigned int)app_state->settings_ingredient_scroll_index,
           (unsigned int)get_settings_ingredient_scroll_max(),
           (int)app_state->selected_settings_pump + 1);
    display_draw_text_5x7(55, 18, "MAPPING", COLOR_WHITE, 4);
    draw_status_bar(title);
    display_draw_text_5x7(20, 72, get_pump_mapping_name(app_state->selected_settings_pump), COLOR_WHITE, 3);
    redraw_settings_list_area(app_state);
  } else {
    char title[32];
    char trial_text[24];
    char saved_text[24];
    const Pump *selected_pump = &pumps[app_state->selected_settings_pump];
    const uint16_t saved_color = selected_pump->calibration_valid ? COLOR_GREEN : COLOR_LIGHTGRAY;

    snprintf(title, sizeof(title), "PUMP %d CAL", (int)app_state->selected_settings_pump + 1);
    snprintf(trial_text, sizeof(trial_text), "TRIAL: %lu",
             (unsigned long)app_state->settings_calibration_trial_ms_per_ml);
    if (selected_pump->calibration_valid) {
      snprintf(saved_text, sizeof(saved_text), "SAVED: %lu",
               (unsigned long)selected_pump->calibration_ms_per_ml);
    } else {
      snprintf(saved_text, sizeof(saved_text), "SAVED: NONE");
    }

    printf("Drawing settings calibration view for pump %d...\r\n",
           (int)app_state->selected_settings_pump + 1);
    display_draw_text_5x7(52, 18, "CAL", COLOR_WHITE, 4);
    draw_status_bar(title);
    display_draw_text_5x7(58, 82, title, COLOR_WHITE, 2);

    display_fill_rect(20, 120, 280, 38, COLOR_DARKGRAY);
    display_draw_text_5x7(28, 132, trial_text, COLOR_WHITE, 2);

    display_fill_rect(20, 172, 280, 38, COLOR_DARKGRAY);
    display_draw_text_5x7(28, 184, saved_text, saved_color, 2);

    display_draw_text_5x7(20, 228, "TEST = SIM", COLOR_WHITE, 2);
    display_draw_text_5x7(20, 252, "PLACEHOLDER", COLOR_WHITE, 2);
    display_draw_text_5x7(20, 332, "ACTIONS", COLOR_WHITE, 2);
    if (app_state->settings_calibration_saved) {
      display_draw_text_5x7(20, 424, "SAVED TO PUMP", COLOR_GREEN, 1);
    }

    for (int i = 0; i < NUM_SETTINGS_CALIBRATION_ZONES; i++) {
      draw_settings_calibration_zone(i, false);
    }
  }
}

static void draw_placeholder_screen(const char *title, const char *status) {
  printf("Drawing placeholder screen: %s\r\n", title);
  display_fill_screen(COLOR_BLACK);
  display_fill_rect(0, 0, LCD_WIDTH, 70, COLOR_BLUE);
  display_draw_text_5x7(20, 18, title, COLOR_WHITE, 4);
  draw_status_bar(status);
  display_draw_text_5x7(70, 170, "COMING", COLOR_WHITE, 4);
  draw_labeled_zone(&menu_back_zone, menu_back_zone.color, COLOR_BLACK, 4);
}

static void draw_detail_zone(int idx, const AppState *app_state, bool active) {
    if (idx < 0 || idx >= NUM_DETAIL_ZONES) return;

    const Zone *z = &detail_zones[idx];
    uint16_t fill_color = z->color;
    uint16_t text_color = (fill_color == COLOR_LIGHTGRAY) ? COLOR_BLACK : COLOR_WHITE;

    if (idx == DETAIL_ZONE_ML_40 && app_state->selected_main_ml == 40) {
      fill_color = COLOR_GREEN;
    } else if (idx == DETAIL_ZONE_ML_60 && app_state->selected_main_ml == 60) {
      fill_color = COLOR_GREEN;
    } else if (idx == DETAIL_ZONE_ML_80 && app_state->selected_main_ml == 80) {
      fill_color = COLOR_GREEN;
    }

    if (active) {
      fill_color = COLOR_WHITE;
      text_color = COLOR_BLUE;
    } else if (fill_color == COLOR_LIGHTGRAY) {
      text_color = COLOR_BLACK;
    }

    draw_labeled_zone(z, fill_color, text_color, 3);
}

static void draw_drink_detail_screen(const AppState *app_state, int active_detail_zone) {
  const char *title = get_selected_label(app_state);
  if (title == NULL) return;

  printf("[DEBUG] draw_drink_detail_screen drink=%s ml=%u active_zone=%d\r\n",
         title,
         app_state->selected_main_ml,
         active_detail_zone);

  display_fill_screen(COLOR_BLACK);
  display_fill_rect(0, 0, LCD_WIDTH, 70, COLOR_RED);
  display_draw_text_5x7(70, 18, "DETAIL", COLOR_WHITE, 5);
  draw_status_bar(title);
  display_draw_text_5x7(20, 90, title, COLOR_WHITE, 4);

  for (int i = 0; i < NUM_DETAIL_ZONES; i++) {
    draw_detail_zone(i, app_state, i == active_detail_zone);
  }
}

static void redraw_detail_ml_controls(const AppState *app_state) {
  draw_detail_zone(DETAIL_ZONE_ML_40, app_state, false);
  draw_detail_zone(DETAIL_ZONE_ML_60, app_state, false);
  draw_detail_zone(DETAIL_ZONE_ML_80, app_state, false);
}

static void draw_pouring_progress_bar(uint16_t fill_width) {
  const uint16_t bar_x = 20;
  const uint16_t bar_y = 220;
  const uint16_t bar_w = 280;
  const uint16_t bar_h = 40;

  display_fill_rect(bar_x, bar_y, bar_w, bar_h, COLOR_LIGHTGRAY);
  if (fill_width > 0) {
    if (fill_width > bar_w) fill_width = bar_w;
    display_fill_rect(bar_x, bar_y, fill_width, bar_h, COLOR_GREEN);
  }
}

static void draw_pouring_screen(const AppState *app_state) {
  printf("[DEBUG] draw_pouring_screen drink=%s ml=%u duration=%lu\r\n",
         get_selected_label(app_state),
         app_state->selected_main_ml,
         (unsigned long)app_state->pour_duration_ms);

  display_fill_screen(COLOR_BLACK);
  display_fill_rect(0, 0, LCD_WIDTH, 70, COLOR_BLUE);
  display_draw_text_5x7(85, 18, "POUR", COLOR_WHITE, 5);
  draw_status_bar(get_selected_label(app_state));

  display_draw_text_5x7(20, 95, get_selected_label(app_state), COLOR_WHITE, 4);

  display_draw_text_5x7(20, 155, "ML", COLOR_WHITE, 3);
  if (app_state->selected_main_ml == 40) {
    display_draw_text_5x7(90, 155, "40", COLOR_WHITE, 3);
  } else if (app_state->selected_main_ml == 60) {
    display_draw_text_5x7(90, 155, "60", COLOR_WHITE, 3);
  } else {
    display_draw_text_5x7(90, 155, "80", COLOR_WHITE, 3);
  }

  draw_pouring_progress_bar(app_state->last_pour_progress_px);
}

static void redraw_current_screen(const AppState *app_state) {
  if (app_state->current_screen == SCREEN_MAIN_MENU) {
    draw_main_menu_screen();
  } else if (app_state->current_screen == SCREEN_DRINK_MENU) {
    draw_drink_menu_screen(app_state);
  } else if (app_state->current_screen == SCREEN_SHOT_MENU) {
    draw_shot_menu_screen(app_state);
  } else if (app_state->current_screen == SCREEN_SETTINGS_MENU) {
    draw_settings_screen(app_state);
  } else if (app_state->current_screen == SCREEN_DRINK_DETAIL) {
    draw_drink_detail_screen(app_state, -1);
  } else if (app_state->current_screen == SCREEN_POURING) {
    draw_pouring_screen(app_state);
  }
}

// Touch functions
static void touch_init(void) {
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

static bool touch_read(uint16_t *x, uint16_t *y) {
  uint8_t data[4];
  uint8_t reg = 0x02; // TD_STATUS
  uint32_t timeout_us = 5000;

  if (i2c_write_timeout_us(i2c1, FT6336U_ADDR, &reg, 1, true, timeout_us) < 0) return false;
  
  uint8_t td_status = 0;
  if (i2c_read_timeout_us(i2c1, FT6336U_ADDR, &td_status, 1, false, timeout_us) < 0) return false;

  uint8_t touch_points = td_status & 0x0F;
  if (touch_points == 0) return false;

  reg = 0x03; // P1_XH
  if (i2c_write_timeout_us(i2c1, FT6336U_ADDR, &reg, 1, true, timeout_us) < 0) return false;
  if (i2c_read_timeout_us(i2c1, FT6336U_ADDR, data, 4, false, timeout_us) < 0) return false;

  *x = ((data[0] & 0x0F) << 8) | data[1];
  *y = ((data[2] & 0x0F) << 8) | data[3];

  return true;
}

int main() {
  stdio_init_all();
  stdio_usb_init();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  sleep_ms(1000);
  for (int i = 0; i < 50; i++) {
    if (stdio_usb_connected()) break;
    sleep_ms(100);
  }

  printf("\r\n=== Tipsy-RP2350 Display + Touch Log ===\r\n");
  fflush(stdout);

  display_init();
  draw_main_menu_screen();
  touch_init();

  AppState app_state = {
    .current_screen = SCREEN_MAIN_MENU,
    .selected_drink = NULL,
    .selected_shot = INGREDIENT_GIN,
    .selected_is_shot = false,
    .selected_main_ml = 0,
    .machine_state = MACHINE_IDLE,
    .pour_started_ms = 0,
    .pour_duration_ms = 0,
    .last_pour_progress_px = 0,
    .pour_complete_logged = false,
    .selected_settings_pump = PUMP_ID_1,
    .drink_menu_scroll_index = 0,
    .shot_menu_scroll_index = 0,
    .settings_pump_scroll_index = 0,
    .settings_pump_scroll_offset_px = 0,
    .settings_pump_drag_active = false,
    .settings_ingredient_scroll_index = 0,
    .settings_view = SETTINGS_VIEW_PUMP_LIST,
    .settings_calibration_trial_ms_per_ml = 1200,
    .settings_calibration_saved = false
  };

  for (size_t i = 0; i < NUM_DRINKS; i++) {
    printf("Drink model: %s | available=%s\r\n",
           drinks[i].name,
           drink_is_available(&drinks[i]) ? "yes" : "no");
  }
  for (int i = 0; i < NUM_INGREDIENTS; i++) {
    const IngredientId shot = (IngredientId)i;
    if (!ingredients[i].is_alcoholic) continue;

    printf("Shot model: %s | available=%s\r\n",
           ingredients[i].name,
           shot_availability_reason(shot, NULL, NULL, NULL) ? "yes" : "no");
  }
  fflush(stdout);

  bool last_touch_state = false;
  uint16_t last_x = 0;
  uint16_t last_y = 0;
  uint16_t touch_start_y = 0;
  bool touch_scroll_candidate = false;
  bool touch_scrolled = false;
  bool needs_settings_list_redraw = false;
  int settings_gesture_start_zone = -1;
  uint32_t loop_count = 0;
  uint32_t screen_log_counter = 0;
  int active_zone = -1; // -1 means no zone
  int pressed_home_zone = -1;
  int pressed_detail_zone = -1;
  bool needs_redraw = false;

  while (true) {
    uint16_t x = 0, y = 0;
    bool current_touch_state = touch_read(&x, &y);
    bool touch_released = !current_touch_state && last_touch_state;
    int new_zone = -1;
    
    if (current_touch_state) {
        if (!last_touch_state) {
            touch_start_y = y;
            touch_scroll_candidate = touch_targets_scrollable_list(&app_state, x, y);
            touch_scrolled = false;
            settings_gesture_start_zone = -1;
            pressed_home_zone = -1;
            pressed_detail_zone = -1;
            if (app_state.current_screen == SCREEN_SETTINGS_MENU &&
                (app_state.settings_view == SETTINGS_VIEW_PUMP_LIST ||
                 app_state.settings_view == SETTINGS_VIEW_MAPPING)) {
                if (app_state.settings_view == SETTINGS_VIEW_PUMP_LIST) {
                    settings_gesture_start_zone =
                        get_settings_pump_item_index_at_point(&app_state, x, y);
                } else {
                    for (int i = 0; i < NUM_SETTINGS_INGREDIENT_ZONES; i++) {
                        if (point_in_zone(x, y, &settings_ingredient_zones[i])) {
                            settings_gesture_start_zone = i;
                            break;
                        }
                    }
                }
                pressed_home_zone = settings_gesture_start_zone;
                new_zone = -1;
                printf("SETTINGS gesture start item=%d y=%u view=%d\r\n",
                       settings_gesture_start_zone,
                       y,
                       app_state.settings_view);
            }
            if (app_state.current_screen == SCREEN_SETTINGS_MENU) {
                printf("SETTINGS touch start y=%u view=%d scroll_candidate=%s pump_index=%u ingredient_index=%u\r\n",
                       y,
                       app_state.settings_view,
                       touch_scroll_candidate ? "yes" : "no",
                       (unsigned int)app_state.settings_pump_scroll_index,
                       (unsigned int)app_state.settings_ingredient_scroll_index);
                printf("SETTINGS scroll candidate=%s x=%u y=%u\r\n",
                       touch_scroll_candidate ? "yes" : "no",
                       x,
                       y);
            }
        } else if (touch_scroll_candidate &&
                   abs((int)y - (int)touch_start_y) > TOUCH_SCROLL_THRESHOLD_PX) {
            const bool settings_scroll_view =
                app_state.current_screen == SCREEN_SETTINGS_MENU &&
                (app_state.settings_view == SETTINGS_VIEW_PUMP_LIST ||
                 app_state.settings_view == SETTINGS_VIEW_MAPPING);
            if (settings_scroll_view) {
                printf("SETTINGS gesture became scroll delta=%d\r\n",
                       (int)y - (int)touch_start_y);
                touch_scrolled = true;
                const bool scroll_changed =
                    app_state.settings_view == SETTINGS_VIEW_PUMP_LIST
                        ? scroll_settings_pump_by_pixels(&app_state,
                                                         (int16_t)y - (int16_t)last_y)
                        : apply_vertical_scroll(&app_state,
                                                (int16_t)y - (int16_t)touch_start_y);
                if (scroll_changed) {
                    if (app_state.settings_view == SETTINGS_VIEW_PUMP_LIST) {
                        app_state.settings_pump_drag_active = true;
                    }
                    if (app_state.settings_view == SETTINGS_VIEW_MAPPING) {
                        touch_start_y = y;
                    }
                    new_zone = -1;
                    pressed_home_zone = -1;
                    pressed_detail_zone = -1;
                    active_zone = -1;
                    needs_settings_list_redraw = true;
                    printf("[DEBUG] touch scroll detected new_anchor_y=%u current_y=%u\r\n",
                           touch_start_y,
                           y);
                }
            } else {
                const bool scroll_changed = apply_vertical_scroll(&app_state, (int16_t)y - (int16_t)touch_start_y);
                if (scroll_changed) {
                    touch_scrolled = true;
                    touch_start_y = y;
                    new_zone = -1;
                    pressed_home_zone = -1;
                    pressed_detail_zone = -1;
                    active_zone = -1;
                    needs_redraw = true;
                    printf("[DEBUG] touch scroll detected new_anchor_y=%u current_y=%u\r\n",
                           touch_start_y,
                           y);
                }
            }
        }

        if (app_state.current_screen == SCREEN_SETTINGS_MENU &&
            (app_state.settings_view == SETTINGS_VIEW_PUMP_LIST ||
             app_state.settings_view == SETTINGS_VIEW_MAPPING)) {
            new_zone = -1;
        } else if (touch_scrolled) {
            new_zone = -1;
        } else if (app_state.current_screen == SCREEN_MAIN_MENU) {
            for (int i = 0; i < NUM_MAIN_MENU_ZONES; i++) {
                if (point_in_zone(x, y, &main_menu_zones[i])) {
                    new_zone = i;
                    pressed_home_zone = i;
                    printf("[DEBUG] MAIN MENU zone detected: %d (%s)\r\n", i, main_menu_zones[i].name);
                    break;
                }
            }
        } else if (app_state.current_screen == SCREEN_DRINK_MENU) {
            for (int i = 0; i < NUM_DRINK_MENU_ZONES; i++) {
                if (point_in_zone(x, y, &drink_menu_zones[i])) {
                    new_zone = i;
                    pressed_home_zone = i;
                    printf("[DEBUG] DRINK MENU zone detected: %d (%s)\r\n", i, drink_menu_zones[i].name);
                    break;
                }
            }
        } else if (app_state.current_screen == SCREEN_SHOT_MENU) {
            for (int i = 0; i < NUM_SHOT_MENU_ZONES; i++) {
                if (point_in_zone(x, y, &shot_menu_zones[i])) {
                    new_zone = i;
                    pressed_home_zone = i;
                    printf("[DEBUG] SHOT MENU zone detected: %d\r\n", i);
                    break;
                }
            }
        } else if (app_state.current_screen == SCREEN_SETTINGS_MENU) {
            if (app_state.settings_view == SETTINGS_VIEW_PUMP_LIST) {
                new_zone = -1;
            } else if (app_state.settings_view == SETTINGS_VIEW_MAPPING) {
                new_zone = -1;
            } else {
                for (int i = 0; i < NUM_SETTINGS_CALIBRATION_ZONES; i++) {
                    if (point_in_zone(x, y, &settings_calibration_zones[i])) {
                        new_zone = i;
                        pressed_home_zone = i;
                        printf("[DEBUG] SETTINGS CAL zone detected: %d\r\n", i);
                        break;
                    }
                }
            }
        } else if (app_state.current_screen == SCREEN_DRINK_DETAIL) {
            for (int i = 0; i < NUM_DETAIL_ZONES; i++) {
                if (point_in_zone(x, y, &detail_zones[i])) {
                    new_zone = i;
                    pressed_detail_zone = i;
                    if (i == DETAIL_ZONE_ML_40) {
                      printf("[DEBUG] DETAIL zone detected: 40\r\n");
                    } else if (i == DETAIL_ZONE_ML_60) {
                      printf("[DEBUG] DETAIL zone detected: 60\r\n");
                    } else if (i == DETAIL_ZONE_ML_80) {
                      printf("[DEBUG] DETAIL zone detected: 80\r\n");
                    } else if (i == DETAIL_ZONE_POUR) {
                      printf("[DEBUG] DETAIL zone detected: POUR\r\n");
                    } else if (i == DETAIL_ZONE_BACK) {
                      printf("[DEBUG] DETAIL zone detected: BACK\r\n");
                    }
                    break;
                }
            }
        } else if (app_state.current_screen == SCREEN_POURING) {
            new_zone = -1;
        }

        if (!last_touch_state) {
            printf("Touch DOWN at (%u, %u)\r\n", x, y);
            fflush(stdout);
        }
        last_x = x;
        last_y = y;
    } else if (!current_touch_state && last_touch_state) {
        printf("Touch UP\r\n");
        fflush(stdout);
    }
    last_touch_state = current_touch_state;

    if (new_zone != active_zone) {
        if (app_state.current_screen == SCREEN_MAIN_MENU) {
            if (active_zone >= 0) draw_zone_from_array(main_menu_zones, NUM_MAIN_MENU_ZONES, active_zone, false);

            if (new_zone >= 0) {
                printf("%s selected\r\n", main_menu_zones[new_zone].name);
                draw_zone_from_array(main_menu_zones, NUM_MAIN_MENU_ZONES, new_zone, true);
            }
        } else if (app_state.current_screen == SCREEN_DRINK_MENU) {
            if (active_zone >= 0 && active_zone < NUM_DRINK_MENU_ZONES) {
                draw_drink_menu_zone(&app_state, active_zone, false);
            }

            if (new_zone >= 0) {
                const Drink *available_drinks[MAX_DRINK_MENU_ITEMS];
                const size_t drink_count = collect_available_drinks(available_drinks, MAX_DRINK_MENU_ITEMS);

                const int visible_drink_index = new_zone + app_state.drink_menu_scroll_index;

                if (new_zone < DRINK_MENU_VISIBLE_ITEMS && visible_drink_index < (int)drink_count) {
                    const Drink *drink = available_drinks[visible_drink_index];
                    printf("%s selected\r\n", drink->name);
                    app_state.selected_is_shot = false;
                    app_state.selected_drink = drink;
                    app_state.selected_main_ml = drink != NULL
                                                  ? normalize_main_spirit_ml(drink->default_main_spirit_ml)
                                                  : 0;
                    app_state.machine_state = (drink != NULL && drink_is_available(drink))
                                                ? MACHINE_DRINK_SELECTED
                                                : MACHINE_ATTENTION_REQUIRED;

                    if (drink != NULL) {
                      printf("Mapped drink: %s | available=%s | main=%u ml | pour=%lu ms\r\n",
                             drink->name,
                             drink_is_available(drink) ? "yes" : "no",
                             app_state.selected_main_ml,
                             (unsigned long)calculate_pour_time_ms(app_state.selected_main_ml));
                    }
                } else if (new_zone == NUM_DRINK_MENU_ZONES - 1) {
                    printf("BACK selected\r\n");
                } else {
                    printf("No available drink mapped for zone %d (scroll=%u)\r\n",
                           new_zone,
                           app_state.drink_menu_scroll_index);
                }

                draw_drink_menu_zone(&app_state, new_zone, true);
            }
        } else if (app_state.current_screen == SCREEN_SHOT_MENU) {
            if (active_zone >= 0 && active_zone < NUM_SHOT_MENU_ZONES) {
                draw_shot_menu_zone(&app_state, active_zone, false);
            }
            if (new_zone >= 0) {
                IngredientId available_shots[MAX_SHOT_MENU_ITEMS];
                const size_t shot_count = collect_available_shots(available_shots, MAX_SHOT_MENU_ITEMS);
                const int visible_shot_index = new_zone + app_state.shot_menu_scroll_index;

                if (new_zone < SHOT_MENU_VISIBLE_ITEMS && visible_shot_index < (int)shot_count) {
                    const IngredientId shot = available_shots[visible_shot_index];
                    app_state.selected_drink = NULL;
                    app_state.selected_shot = shot;
                    app_state.selected_is_shot = true;
                    app_state.selected_main_ml = 40;
                    app_state.machine_state = MACHINE_DRINK_SELECTED;
                    printf("Shot selected: %s | main=%u ml | pour=%lu ms\r\n",
                           get_shot_label(shot),
                           app_state.selected_main_ml,
                           (unsigned long)calculate_pour_time_ms(app_state.selected_main_ml));
                } else {
                    printf("SHOT BACK selected\r\n");
                }

                draw_shot_menu_zone(&app_state, new_zone, true);
            }
        } else if (app_state.current_screen == SCREEN_DRINK_DETAIL) {
            if (active_zone >= 0 && active_zone < NUM_DETAIL_ZONES) {
                draw_detail_zone(active_zone, &app_state, false);
            }
            if (new_zone >= 0) {
                draw_detail_zone(new_zone, &app_state, true);
            }
        } else if (app_state.current_screen == SCREEN_SETTINGS_MENU) {
            if (active_zone >= 0) {
                if (app_state.settings_view == SETTINGS_VIEW_PUMP_LIST) {
                    draw_settings_pump_zone(&app_state, active_zone, false);
                } else if (app_state.settings_view == SETTINGS_VIEW_MAPPING) {
                    draw_settings_edit_zone(&app_state, active_zone, false);
                } else {
                    draw_settings_calibration_zone(active_zone, false);
                }
            }
            if (new_zone >= 0) {
                if (app_state.settings_view == SETTINGS_VIEW_PUMP_LIST) {
                    draw_settings_pump_zone(&app_state, new_zone, true);
                } else if (app_state.settings_view == SETTINGS_VIEW_MAPPING) {
                    draw_settings_edit_zone(&app_state, new_zone, true);
                } else {
                    draw_settings_calibration_zone(new_zone, true);
                }
            }
        }
        
        fflush(stdout);
        active_zone = new_zone;
    }

    if (touch_released && touch_scrolled) {
        if (app_state.current_screen == SCREEN_SETTINGS_MENU &&
            (app_state.settings_view == SETTINGS_VIEW_PUMP_LIST ||
             app_state.settings_view == SETTINGS_VIEW_MAPPING)) {
            printf("SETTINGS tap suppressed due to scroll\r\n");
            if (app_state.settings_view == SETTINGS_VIEW_PUMP_LIST &&
                app_state.settings_pump_drag_active) {
                app_state.settings_pump_drag_active = false;
                needs_settings_list_redraw = true;
            }
        }
        pressed_home_zone = -1;
        pressed_detail_zone = -1;
        active_zone = -1;
        settings_gesture_start_zone = -1;
    } else if (touch_released && app_state.current_screen == SCREEN_MAIN_MENU && pressed_home_zone >= 0) {
        if (pressed_home_zone == 0) {
            printf("[DEBUG] SCREEN_DRINK_MENU set from MAIN_MENU\r\n");
            app_state.current_screen = SCREEN_DRINK_MENU;
            needs_redraw = true;
        } else if (pressed_home_zone == 1) {
            printf("[DEBUG] SCREEN_SHOT_MENU set from MAIN_MENU\r\n");
            app_state.current_screen = SCREEN_SHOT_MENU;
            needs_redraw = true;
        } else if (pressed_home_zone == 2) {
            printf("[DEBUG] SCREEN_SETTINGS_MENU set from MAIN_MENU\r\n");
            app_state.current_screen = SCREEN_SETTINGS_MENU;
            app_state.settings_view = SETTINGS_VIEW_PUMP_LIST;
            app_state.settings_pump_scroll_index = 0;
            app_state.settings_pump_scroll_offset_px = 0;
            app_state.settings_pump_drag_active = false;
            app_state.settings_ingredient_scroll_index = 0;
            app_state.settings_calibration_saved = false;
            app_state.settings_calibration_trial_ms_per_ml = get_pump_calibration_ms_per_ml(app_state.selected_settings_pump);
            needs_redraw = true;
        }
        fflush(stdout);
        pressed_home_zone = -1;
        active_zone = -1;
    } else if (touch_released && app_state.current_screen == SCREEN_DRINK_MENU && pressed_home_zone >= 0) {
        const Drink *available_drinks[MAX_DRINK_MENU_ITEMS];
        const size_t drink_count = collect_available_drinks(available_drinks, MAX_DRINK_MENU_ITEMS);

        const int visible_drink_index = pressed_home_zone + app_state.drink_menu_scroll_index;

        if (pressed_home_zone < DRINK_MENU_VISIBLE_ITEMS &&
            visible_drink_index < (int)drink_count) {
            app_state.selected_is_shot = false;
            printf("[DEBUG] SCREEN_DRINK_DETAIL set for drink=%s via drink_menu_zone=%d\r\n",
                   app_state.selected_drink != NULL ? app_state.selected_drink->name : "none",
                   pressed_home_zone);
            app_state.current_screen = SCREEN_DRINK_DETAIL;
            needs_redraw = true;
            printf("Opened detail: %s | selected ml=%u\r\n",
                   app_state.selected_drink != NULL ? app_state.selected_drink->name : "none",
                   app_state.selected_main_ml);
        } else if (pressed_home_zone == NUM_DRINK_MENU_ZONES - 1) {
            printf("[DEBUG] SCREEN_MAIN_MENU set from DRINK_MENU BACK\r\n");
            app_state.current_screen = SCREEN_MAIN_MENU;
            needs_redraw = true;
        }
        fflush(stdout);
        pressed_home_zone = -1;
        active_zone = -1;
    } else if (touch_released && app_state.current_screen == SCREEN_SHOT_MENU && pressed_home_zone >= 0) {
        IngredientId available_shots[MAX_SHOT_MENU_ITEMS];
        const size_t shot_count = collect_available_shots(available_shots, MAX_SHOT_MENU_ITEMS);

        const int visible_shot_index = pressed_home_zone + app_state.shot_menu_scroll_index;

        if (pressed_home_zone < SHOT_MENU_VISIBLE_ITEMS &&
            visible_shot_index < (int)shot_count) {
            printf("[DEBUG] SCREEN_DRINK_DETAIL set for shot=%s via shot_menu_zone=%d\r\n",
                   get_selected_label(&app_state),
                   pressed_home_zone);
            app_state.current_screen = SCREEN_DRINK_DETAIL;
            needs_redraw = true;
            printf("Opened detail: %s | selected ml=%u\r\n",
                   get_selected_label(&app_state),
                   app_state.selected_main_ml);
        } else {
            printf("[DEBUG] SCREEN_MAIN_MENU set from SHOT_MENU BACK\r\n");
            app_state.current_screen = SCREEN_MAIN_MENU;
            needs_redraw = true;
        }
        fflush(stdout);
        pressed_home_zone = -1;
        active_zone = -1;
    } else if (touch_released &&
               app_state.current_screen == SCREEN_SETTINGS_MENU &&
               pressed_home_zone >= 0) {
        printf("SETTINGS tap item=%d view=%d\r\n", pressed_home_zone, app_state.settings_view);
        if (app_state.settings_view == SETTINGS_VIEW_PUMP_LIST) {
            SettingsItem items[SETTINGS_MAIN_ITEM_COUNT];
            const size_t item_count = build_settings_items(items, SETTINGS_MAIN_ITEM_COUNT);
            const int item_index = pressed_home_zone;

            if (pressed_home_zone >= 0 &&
                item_index < (int)item_count) {
                const SettingsItem *item = &items[item_index];
                printf("SETTINGS item pressed index=%d label=%s type=%d\r\n",
                       item_index,
                       item->label,
                       item->type);

                if (item->type == SETTINGS_ITEM_PUMP) {
                    app_state.selected_settings_pump = item->pump_id;
                    app_state.settings_pump_drag_active = false;
                    app_state.settings_view = SETTINGS_VIEW_MAPPING;
                    app_state.settings_ingredient_scroll_index = 0;
                    printf("[DEBUG] SETTINGS mapping pump=%d from list item\r\n",
                           (int)app_state.selected_settings_pump + 1);
                    needs_redraw = true;
                } else if (item->type == SETTINGS_ITEM_CALIBRATION) {
                    app_state.settings_pump_drag_active = false;
                    app_state.settings_view = SETTINGS_VIEW_CALIBRATION;
                    app_state.settings_calibration_trial_ms_per_ml =
                        get_pump_calibration_ms_per_ml(app_state.selected_settings_pump);
                    app_state.settings_calibration_saved = false;
                    printf("[DEBUG] SETTINGS calibration pump=%d trial=%lu from list item\r\n",
                           (int)app_state.selected_settings_pump + 1,
                           (unsigned long)app_state.settings_calibration_trial_ms_per_ml);
                    needs_redraw = true;
                } else if (item->type == SETTINGS_ITEM_BACK) {
                    printf("[DEBUG] SCREEN_MAIN_MENU set from SETTINGS list BACK\r\n");
                    app_state.current_screen = SCREEN_MAIN_MENU;
                    needs_redraw = true;
                }
            }
        } else if (app_state.settings_view == SETTINGS_VIEW_MAPPING) {
            const int ingredient_start = app_state.settings_ingredient_scroll_index;

            if (pressed_home_zone < SETTINGS_INGREDIENT_VISIBLE_ITEMS &&
                (ingredient_start + pressed_home_zone) < NUM_INGREDIENTS) {
                const IngredientId new_ingredient = (IngredientId)(ingredient_start + pressed_home_zone);
                apply_pump_mapping(app_state.selected_settings_pump, new_ingredient);
                app_state.settings_pump_drag_active = false;
                app_state.settings_view = SETTINGS_VIEW_PUMP_LIST;
                needs_redraw = true;
            } else if (pressed_home_zone == SETTINGS_INGREDIENT_ZONE_BACK) {
                app_state.settings_pump_drag_active = false;
                app_state.settings_view = SETTINGS_VIEW_PUMP_LIST;
                needs_redraw = true;
            }
        } else {
            if (pressed_home_zone == 0 && app_state.selected_settings_pump > PUMP_ID_1) {
                app_state.selected_settings_pump--;
                app_state.settings_calibration_trial_ms_per_ml =
                    get_pump_calibration_ms_per_ml(app_state.selected_settings_pump);
                app_state.settings_calibration_saved = false;
                printf("[DEBUG] SETTINGS calibration prev pump=%d\r\n", (int)app_state.selected_settings_pump + 1);
                needs_redraw = true;
            } else if (pressed_home_zone == 1 && app_state.selected_settings_pump + 1 < NUM_PUMPS) {
                app_state.selected_settings_pump++;
                app_state.settings_calibration_trial_ms_per_ml =
                    get_pump_calibration_ms_per_ml(app_state.selected_settings_pump);
                app_state.settings_calibration_saved = false;
                printf("[DEBUG] SETTINGS calibration next pump=%d\r\n", (int)app_state.selected_settings_pump + 1);
                needs_redraw = true;
            } else if (pressed_home_zone == 2) {
                app_state.settings_calibration_trial_ms_per_ml =
                    1200u + ((uint32_t)app_state.selected_settings_pump * 200u);
                app_state.settings_calibration_saved = false;
                printf("Calibration test simulated: Pump %d | trial=%lu ms/ml\r\n",
                       (int)app_state.selected_settings_pump + 1,
                       (unsigned long)app_state.settings_calibration_trial_ms_per_ml);
                needs_redraw = true;
            } else if (pressed_home_zone == 3) {
                pumps[app_state.selected_settings_pump].calibration_ms_per_ml =
                    app_state.settings_calibration_trial_ms_per_ml;
                pumps[app_state.selected_settings_pump].calibration_valid = true;
                app_state.settings_calibration_saved = true;
                printf("Calibration saved: Pump %d | value=%lu ms/ml\r\n",
                       (int)app_state.selected_settings_pump + 1,
                       (unsigned long)pumps[app_state.selected_settings_pump].calibration_ms_per_ml);
                needs_redraw = true;
            } else if (pressed_home_zone == 4) {
                app_state.settings_view = SETTINGS_VIEW_PUMP_LIST;
                app_state.settings_pump_drag_active = false;
                app_state.settings_calibration_saved = false;
                needs_redraw = true;
            }
        }
        fflush(stdout);
        pressed_home_zone = -1;
        active_zone = -1;
        settings_gesture_start_zone = -1;
    } else if (touch_released && app_state.current_screen == SCREEN_DRINK_DETAIL && pressed_detail_zone >= 0) {
        active_zone = pressed_detail_zone;
        if (active_zone == DETAIL_ZONE_ML_40) {
            app_state.selected_main_ml = 40;
            printf("[DEBUG] DETAIL action: 40 ml selected\r\n");
            printf("Main spirit set: %u ml\r\n", app_state.selected_main_ml);
            redraw_detail_ml_controls(&app_state);
        } else if (active_zone == DETAIL_ZONE_ML_60) {
            app_state.selected_main_ml = 60;
            printf("[DEBUG] DETAIL action: 60 ml selected\r\n");
            printf("Main spirit set: %u ml\r\n", app_state.selected_main_ml);
            redraw_detail_ml_controls(&app_state);
        } else if (active_zone == DETAIL_ZONE_ML_80) {
            app_state.selected_main_ml = 80;
            printf("[DEBUG] DETAIL action: 80 ml selected\r\n");
            printf("Main spirit set: %u ml\r\n", app_state.selected_main_ml);
            redraw_detail_ml_controls(&app_state);
        } else if (active_zone == DETAIL_ZONE_POUR &&
                   (app_state.selected_drink != NULL || app_state.selected_is_shot)) {
            printf("[DEBUG] DETAIL action: POUR\r\n");
            printf("Pour requested: selection=%s | ml=%u | pour=%lu ms\r\n",
                   get_selected_label(&app_state),
                   app_state.selected_main_ml,
                   (unsigned long)calculate_pour_time_ms(app_state.selected_main_ml));
            app_state.current_screen = SCREEN_POURING;
            app_state.machine_state = MACHINE_DRINK_SELECTED;
            app_state.pour_duration_ms = calculate_pour_time_ms(app_state.selected_main_ml);
            app_state.pour_started_ms = to_ms_since_boot(get_absolute_time());
            app_state.last_pour_progress_px = 0;
            app_state.pour_complete_logged = false;
            needs_redraw = true;
        } else if (active_zone == DETAIL_ZONE_BACK) {
            app_state.current_screen = app_state.selected_is_shot ? SCREEN_SHOT_MENU : SCREEN_DRINK_MENU;
            printf("[DEBUG] %s set from DETAIL BACK\r\n",
                   app_state.current_screen == SCREEN_SHOT_MENU ? "SCREEN_SHOT_MENU" : "SCREEN_DRINK_MENU");
            needs_redraw = true;
            printf("Back to %s menu\r\n", app_state.selected_is_shot ? "shot" : "drink");
        }

        fflush(stdout);
        pressed_detail_zone = -1;
        active_zone = -1;
    }

    if (app_state.current_screen == SCREEN_POURING) {
        const uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        const uint32_t elapsed_ms = now_ms - app_state.pour_started_ms;
        const uint32_t duration_ms = app_state.pour_duration_ms > 0 ? app_state.pour_duration_ms : 1;
        uint16_t progress_px = (uint16_t)((elapsed_ms >= duration_ms)
                                ? 280
                                : (elapsed_ms * 280u) / duration_ms);

        if (progress_px != app_state.last_pour_progress_px) {
            draw_pouring_progress_bar(progress_px);
            app_state.last_pour_progress_px = progress_px;
        }

        if (elapsed_ms >= duration_ms && !app_state.pour_complete_logged) {
            printf("Pour complete: selection=%s | ml=%u\r\n",
                   get_selected_label(&app_state),
                   app_state.selected_main_ml);
            printf("[DEBUG] SCREEN_MAIN_MENU set after pouring complete\r\n");
            fflush(stdout);
            app_state.pour_complete_logged = true;
            app_state.current_screen = SCREEN_MAIN_MENU;
            needs_redraw = true;
            active_zone = -1;
        }
    }

    if (needs_redraw) {
        printf("[DEBUG] redraw screen=%s\r\n",
               app_state.current_screen == SCREEN_MAIN_MENU ? "MAIN_MENU"
               : (app_state.current_screen == SCREEN_DRINK_MENU ? "DRINK_MENU"
               : (app_state.current_screen == SCREEN_SHOT_MENU ? "SHOT_MENU"
               : (app_state.current_screen == SCREEN_SETTINGS_MENU ? "SETTINGS_MENU"
               : (app_state.current_screen == SCREEN_DRINK_DETAIL ? "DETAIL" : "POURING")))));
        redraw_current_screen(&app_state);
        fflush(stdout);
        needs_redraw = false;
        needs_settings_list_redraw = false;
    } else if (needs_settings_list_redraw) {
        redraw_settings_list_area(&app_state);
        fflush(stdout);
        needs_settings_list_redraw = false;
    }

    loop_count++;
    screen_log_counter++;
    if (screen_log_counter >= 20) {
        printf("[DEBUG] loop screen=%s selected=%s ml=%u active_zone=%d\r\n",
               app_state.current_screen == SCREEN_MAIN_MENU ? "MAIN_MENU"
               : (app_state.current_screen == SCREEN_DRINK_MENU ? "DRINK_MENU"
               : (app_state.current_screen == SCREEN_SHOT_MENU ? "SHOT_MENU"
               : (app_state.current_screen == SCREEN_SETTINGS_MENU ? "SETTINGS_MENU"
               : (app_state.current_screen == SCREEN_DRINK_DETAIL ? "DETAIL" : "POURING")))),
               get_selected_label(&app_state),
               app_state.selected_main_ml,
               active_zone);
        fflush(stdout);
        screen_log_counter = 0;
    }
    if (loop_count >= 10) { // Every 500ms
        gpio_put(LED_PIN, !gpio_get(LED_PIN)); // Toggle LED
        printf("alive\r\n");
        fflush(stdout);
        loop_count = 0;
    }

    sleep_ms(10);
  }

  return 0;
}
