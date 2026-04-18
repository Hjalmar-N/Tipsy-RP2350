#ifndef TIPSY_APP_H
#define TIPSY_APP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
  UI_REFACTOR_SCREEN_BOOT = 0,
  UI_REFACTOR_SCREEN_MAIN_MENU,
  UI_REFACTOR_SCREEN_DRINKS_LIST,
  UI_REFACTOR_SCREEN_SHOTS_LIST,
  UI_REFACTOR_SCREEN_SETTINGS_LIST,
  UI_REFACTOR_SCREEN_SETTINGS_MAPPING,
  UI_REFACTOR_SCREEN_DETAIL,
  UI_REFACTOR_SCREEN_POURING
} UiRefactorScreen;

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
  IngredientId ingredient_id;
  const char *label;
  bool is_back;
} MappingItem;

typedef struct {
  const char *label;
  bool is_back;
} MenuListItem;

typedef struct {
  UiRefactorScreen current_screen;
  PumpId selected_settings_pump;
  uint16_t drinks_scroll_offset_px;
  uint16_t shots_scroll_offset_px;
  uint16_t settings_scroll_offset_px;
  uint16_t mapping_scroll_offset_px;
  int8_t selected_drink_index;
  IngredientId selected_shot;
  bool selected_is_shot;
  uint16_t selected_main_ml;
  uint32_t pour_started_ms;
  uint32_t pour_duration_ms;
  uint16_t last_pour_progress_px;
  bool pour_complete_logged;
  char status_line[48];
} TipsyAppState;

void tipsy_app_init(TipsyAppState *app_state);
void tipsy_app_finish_boot(TipsyAppState *app_state);
void tipsy_app_activate_main_menu(TipsyAppState *app_state, uint8_t menu_index);
void tipsy_app_activate_drinks_item(TipsyAppState *app_state, size_t item_index);
void tipsy_app_activate_shots_item(TipsyAppState *app_state, size_t item_index);
void tipsy_app_activate_settings_item(TipsyAppState *app_state, size_t item_index);
void tipsy_app_activate_mapping_item(TipsyAppState *app_state, size_t item_index);
void tipsy_app_select_ml(TipsyAppState *app_state, uint16_t ml);
void tipsy_app_start_pour(TipsyAppState *app_state, uint32_t now_ms);
void tipsy_app_back_from_detail(TipsyAppState *app_state);
bool tipsy_app_update(TipsyAppState *app_state, uint32_t now_ms);

size_t tipsy_app_build_drinks_items(MenuListItem *out, size_t max_items);
size_t tipsy_app_build_shots_items(MenuListItem *out, size_t max_items);
size_t tipsy_app_build_settings_items(SettingsItem *out, size_t max_items);
size_t tipsy_app_build_mapping_items(MappingItem *out, size_t max_items);
const char *tipsy_get_ingredient_name(IngredientId ingredient_id);
const char *tipsy_get_pump_mapping_name(PumpId pump_id);
const char *tipsy_get_shot_label(IngredientId ingredient_id);
uint32_t tipsy_get_pump_calibration_ms_per_ml(PumpId pump_id);
const Pump *tipsy_get_pumps(void);
size_t tipsy_get_pump_count(void);
const char *tipsy_app_get_selected_label(const TipsyAppState *app_state);
uint16_t tipsy_app_get_selected_main_ml(const TipsyAppState *app_state);
uint16_t tipsy_app_get_pour_progress_px(const TipsyAppState *app_state);

#endif
