#include "tipsy_app.h"

#include <stdio.h>
#include <string.h>

static const uint32_t default_ms_per_ml = 1200;

static Ingredient ingredients[NUM_INGREDIENTS] = {
    {"Gin", true, PUMP_ID_1, true},
    {"Tonic", false, PUMP_ID_2, true},
    {"Campari", true, PUMP_ID_3, true},
    {"Sweet Vermouth", true, PUMP_ID_4, true},
    {"Aperol", true, PUMP_ID_5, true},
    {"Prosecco", true, PUMP_ID_6, true},
    {"Soda", false, PUMP_ID_7, true},
    {"Limoncello", true, PUMP_ID_NONE, true},
};

static Pump pumps[NUM_PUMPS] = {
    {PUMP_ID_1, INGREDIENT_GIN, true, 1200, true},
    {PUMP_ID_2, INGREDIENT_TONIC, true, 1200, true},
    {PUMP_ID_3, INGREDIENT_CAMPARI, true, 1200, true},
    {PUMP_ID_4, INGREDIENT_SWEET_VERMOUTH, true, 1200, true},
    {PUMP_ID_5, INGREDIENT_APEROL, true, 1200, true},
    {PUMP_ID_6, INGREDIENT_PROSECCO, true, 1200, true},
    {PUMP_ID_7, INGREDIENT_SODA, false, 1200, true},
};

static const Drink drinks[] = {
    {"GT", {INGREDIENT_GIN, INGREDIENT_TONIC}, 2, INGREDIENT_GIN, 50},
    {"Negroni", {INGREDIENT_GIN, INGREDIENT_CAMPARI, INGREDIENT_SWEET_VERMOUTH}, 3, INGREDIENT_GIN, 30},
    {"Aperol", {INGREDIENT_APEROL, INGREDIENT_PROSECCO, INGREDIENT_SODA}, 3, INGREDIENT_APEROL, 60},
    {"Limoncello", {INGREDIENT_LIMONCELLO, INGREDIENT_PROSECCO, INGREDIENT_SODA}, 3, INGREDIENT_LIMONCELLO, 60},
};

#define NUM_DRINKS (sizeof(drinks) / sizeof(drinks[0]))

const Pump *tipsy_get_pumps(void) { return pumps; }

size_t tipsy_get_pump_count(void) { return NUM_PUMPS; }

const char *tipsy_get_ingredient_name(IngredientId ingredient_id) {
  if (ingredient_id < 0 || ingredient_id >= NUM_INGREDIENTS) {
    return "NONE";
  }
  return ingredients[ingredient_id].name;
}

const char *tipsy_get_shot_label(IngredientId ingredient_id) {
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

static const Pump *find_pump(PumpId pump_id) {
  if (pump_id < 0 || pump_id >= NUM_PUMPS) {
    return NULL;
  }
  return &pumps[pump_id];
}

const char *tipsy_get_pump_mapping_name(PumpId pump_id) {
  const Pump *pump = find_pump(pump_id);
  if (pump == NULL) {
    return "NONE";
  }
  return tipsy_get_ingredient_name(pump->mapped_ingredient);
}

uint32_t tipsy_get_pump_calibration_ms_per_ml(PumpId pump_id) {
  const Pump *pump = find_pump(pump_id);
  if (pump == NULL || !pump->calibration_valid) {
    return default_ms_per_ml;
  }
  return pump->calibration_ms_per_ml;
}

static bool ingredient_is_available(IngredientId ingredient_id) {
  const Ingredient *ingredient = NULL;
  const Pump *pump = NULL;

  if (ingredient_id < 0 || ingredient_id >= NUM_INGREDIENTS) {
    return false;
  }

  ingredient = &ingredients[ingredient_id];
  if (!ingredient->enabled || ingredient->mapped_pump_id == PUMP_ID_NONE) {
    return false;
  }

  pump = find_pump(ingredient->mapped_pump_id);
  if (pump == NULL || !pump->enabled) {
    return false;
  }

  return pump->mapped_ingredient == ingredient_id;
}

static bool drink_is_available(const Drink *drink) {
  if (drink == NULL) {
    return false;
  }

  for (uint8_t i = 0; i < drink->ingredient_count; ++i) {
    if (!ingredient_is_available(drink->ingredients[i])) {
      return false;
    }
  }

  return true;
}

static bool shot_is_available(IngredientId ingredient_id) {
  if (ingredient_id < 0 || ingredient_id >= NUM_INGREDIENTS) {
    return false;
  }
  if (!ingredients[ingredient_id].is_alcoholic) {
    return false;
  }
  return ingredient_is_available(ingredient_id);
}

void tipsy_app_init(TipsyAppState *app_state) {
  if (app_state == NULL) {
    return;
  }

  memset(app_state, 0, sizeof(*app_state));
  app_state->current_screen = UI_REFACTOR_SCREEN_BOOT;
  app_state->selected_settings_pump = PUMP_ID_1;
  app_state->drinks_scroll_offset_px = 0;
  app_state->shots_scroll_offset_px = 0;
  app_state->settings_scroll_offset_px = 0;
  app_state->mapping_scroll_offset_px = 0;
  snprintf(app_state->status_line, sizeof(app_state->status_line), "Booting UI refactor");
}

void tipsy_app_finish_boot(TipsyAppState *app_state) {
  if (app_state == NULL) {
    return;
  }

  app_state->current_screen = UI_REFACTOR_SCREEN_MAIN_MENU;
  snprintf(app_state->status_line, sizeof(app_state->status_line), "Touch SETTINGS to continue");
}

size_t tipsy_app_build_drinks_items(MenuListItem *out, size_t max_items) {
  size_t count = 0;

  if (out == NULL || max_items == 0) {
    return 0;
  }

  for (size_t i = 0; i < NUM_DRINKS && count < max_items; ++i) {
    if (!drink_is_available(&drinks[i])) {
      continue;
    }
    out[count].label = drinks[i].name;
    out[count].is_back = false;
    ++count;
  }

  if (count < max_items) {
    out[count].label = "Back";
    out[count].is_back = true;
    ++count;
  }

  return count;
}

size_t tipsy_app_build_shots_items(MenuListItem *out, size_t max_items) {
  size_t count = 0;

  if (out == NULL || max_items == 0) {
    return 0;
  }

  for (int i = 0; i < NUM_INGREDIENTS && count < max_items; ++i) {
    if (!shot_is_available((IngredientId)i)) {
      continue;
    }
    out[count].label = tipsy_get_shot_label((IngredientId)i);
    out[count].is_back = false;
    ++count;
  }

  if (count < max_items) {
    out[count].label = "Back";
    out[count].is_back = true;
    ++count;
  }

  return count;
}

size_t tipsy_app_build_settings_items(SettingsItem *out, size_t max_items) {
  static char labels[NUM_PUMPS][32];
  size_t count = 0;

  if (out == NULL || max_items == 0) {
    return 0;
  }

  for (int i = 0; i < NUM_PUMPS && count < max_items; ++i) {
    snprintf(labels[i], sizeof(labels[i]), "Pump %d -> %s", i + 1,
             tipsy_get_pump_mapping_name((PumpId)i));
    out[count].type = SETTINGS_ITEM_PUMP;
    out[count].pump_id = (PumpId)i;
    out[count].label = labels[i];
    ++count;
  }

  if (count < max_items) {
    out[count].type = SETTINGS_ITEM_BACK;
    out[count].pump_id = PUMP_ID_NONE;
    out[count].label = "Back";
    ++count;
  }

  return count;
}

size_t tipsy_app_build_mapping_items(MappingItem *out, size_t max_items) {
  size_t count = 0;

  if (out == NULL || max_items == 0) {
    return 0;
  }

  for (int i = 0; i < NUM_INGREDIENTS && count < max_items; ++i) {
    out[count].ingredient_id = (IngredientId)i;
    out[count].label = tipsy_get_ingredient_name((IngredientId)i);
    out[count].is_back = false;
    ++count;
  }

  if (count < max_items) {
    out[count].ingredient_id = INGREDIENT_GIN;
    out[count].label = "Back";
    out[count].is_back = true;
    ++count;
  }

  return count;
}

static void apply_pump_mapping(PumpId pump_id, IngredientId ingredient_id) {
  Pump *pump = NULL;
  IngredientId old_ingredient_id = INGREDIENT_GIN;
  PumpId other_pump_id = PUMP_ID_NONE;

  if (pump_id < 0 || pump_id >= NUM_PUMPS) {
    return;
  }
  if (ingredient_id < 0 || ingredient_id >= NUM_INGREDIENTS) {
    return;
  }

  pump = &pumps[pump_id];
  old_ingredient_id = pump->mapped_ingredient;
  other_pump_id = ingredients[ingredient_id].mapped_pump_id;

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
}

void tipsy_app_activate_main_menu(TipsyAppState *app_state, uint8_t menu_index) {
  if (app_state == NULL) {
    return;
  }

  if (menu_index == 2) {
    app_state->current_screen = UI_REFACTOR_SCREEN_SETTINGS_LIST;
    app_state->settings_scroll_offset_px = 0;
    snprintf(app_state->status_line, sizeof(app_state->status_line),
             "Settings list opened");
    return;
  }

  if (menu_index == 0) {
    app_state->current_screen = UI_REFACTOR_SCREEN_DRINKS_LIST;
    app_state->drinks_scroll_offset_px = 0;
    snprintf(app_state->status_line, sizeof(app_state->status_line),
             "Drinks list opened");
    return;
  }

  if (menu_index == 1) {
    app_state->current_screen = UI_REFACTOR_SCREEN_SHOTS_LIST;
    app_state->shots_scroll_offset_px = 0;
    snprintf(app_state->status_line, sizeof(app_state->status_line),
             "Shots list opened");
    return;
  }

  snprintf(app_state->status_line, sizeof(app_state->status_line),
           "Menu %u stays on old path for now", (unsigned int)menu_index + 1);
}

void tipsy_app_activate_drinks_item(TipsyAppState *app_state, size_t item_index) {
  MenuListItem items[NUM_DRINKS + 1];
  const size_t item_count = tipsy_app_build_drinks_items(items, NUM_DRINKS + 1);

  if (app_state == NULL || item_index >= item_count) {
    return;
  }

  if (items[item_index].is_back) {
    app_state->current_screen = UI_REFACTOR_SCREEN_MAIN_MENU;
    app_state->drinks_scroll_offset_px = 0;
    snprintf(app_state->status_line, sizeof(app_state->status_line),
             "Returned to main menu");
    return;
  }

  snprintf(app_state->status_line, sizeof(app_state->status_line),
           "Drink selected: %s", items[item_index].label);
}

void tipsy_app_activate_shots_item(TipsyAppState *app_state, size_t item_index) {
  MenuListItem items[NUM_INGREDIENTS + 1];
  const size_t item_count = tipsy_app_build_shots_items(items, NUM_INGREDIENTS + 1);

  if (app_state == NULL || item_index >= item_count) {
    return;
  }

  if (items[item_index].is_back) {
    app_state->current_screen = UI_REFACTOR_SCREEN_MAIN_MENU;
    app_state->shots_scroll_offset_px = 0;
    snprintf(app_state->status_line, sizeof(app_state->status_line),
             "Returned to main menu");
    return;
  }

  snprintf(app_state->status_line, sizeof(app_state->status_line),
           "Shot selected: %s", items[item_index].label);
}

void tipsy_app_activate_settings_item(TipsyAppState *app_state, size_t item_index) {
  SettingsItem items[NUM_PUMPS + 1];
  const size_t item_count = tipsy_app_build_settings_items(items, NUM_PUMPS + 1);

  if (app_state == NULL || item_index >= item_count) {
    return;
  }

  if (items[item_index].type == SETTINGS_ITEM_PUMP) {
    app_state->selected_settings_pump = items[item_index].pump_id;
    app_state->current_screen = UI_REFACTOR_SCREEN_SETTINGS_MAPPING;
    app_state->mapping_scroll_offset_px = 0;
    snprintf(app_state->status_line, sizeof(app_state->status_line),
             "Pump %d mapping",
             (int)items[item_index].pump_id + 1);
  } else {
    app_state->current_screen = UI_REFACTOR_SCREEN_MAIN_MENU;
    app_state->settings_scroll_offset_px = 0;
    snprintf(app_state->status_line, sizeof(app_state->status_line),
             "Returned to main menu");
  }
}

void tipsy_app_activate_mapping_item(TipsyAppState *app_state, size_t item_index) {
  MappingItem items[NUM_INGREDIENTS + 1];
  const size_t item_count = tipsy_app_build_mapping_items(items, NUM_INGREDIENTS + 1);

  if (app_state == NULL || item_index >= item_count) {
    return;
  }

  if (items[item_index].is_back) {
    app_state->current_screen = UI_REFACTOR_SCREEN_SETTINGS_LIST;
    app_state->mapping_scroll_offset_px = 0;
    snprintf(app_state->status_line, sizeof(app_state->status_line),
             "Returned to settings");
    return;
  }

  apply_pump_mapping(app_state->selected_settings_pump, items[item_index].ingredient_id);
  app_state->current_screen = UI_REFACTOR_SCREEN_SETTINGS_LIST;
  app_state->mapping_scroll_offset_px = 0;
  snprintf(app_state->status_line, sizeof(app_state->status_line),
           "Pump %d -> %s",
           (int)app_state->selected_settings_pump + 1,
           tipsy_get_ingredient_name(items[item_index].ingredient_id));
}
