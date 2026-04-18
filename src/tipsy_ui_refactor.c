#include "tipsy_ui_refactor.h"

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "tipsy_hal.h"

typedef struct {
  TipsyRect rect;
  uint16_t fill_color;
  uint16_t text_color;
  const char *label;
} UiButton;

static void draw_header(const char *title);
static void render_settings_scrollbar(size_t item_count,
                                      uint16_t scroll_offset_px);
static void render_detail_screen(const TipsyAppState *app_state);
static void render_pouring_screen(const TipsyAppState *app_state);

static const UiButton main_menu_buttons[3] = {
    {{24, 96, 272, 78}, TIPSY_COLOR_BLUE, TIPSY_COLOR_WHITE, "DRINKS"},
    {{24, 192, 272, 78}, TIPSY_COLOR_GREEN, TIPSY_COLOR_WHITE, "SHOTS"},
    {{24, 288, 272, 78}, TIPSY_COLOR_LIGHTGRAY, TIPSY_COLOR_BLACK, "SETTINGS"},
};

static const UiButton detail_ml_buttons[3] = {
    {{20, 150, 80, 70}, TIPSY_COLOR_BLUE, TIPSY_COLOR_WHITE, "40"},
    {{120, 150, 80, 70}, TIPSY_COLOR_BLUE, TIPSY_COLOR_WHITE, "60"},
    {{220, 150, 80, 70}, TIPSY_COLOR_BLUE, TIPSY_COLOR_WHITE, "80"},
};

static const UiButton detail_pour_button = {
    {20, 260, 180, 80}, TIPSY_COLOR_GREEN, TIPSY_COLOR_BLACK, "POUR"};
static const UiButton detail_back_button = {
    {220, 260, 80, 80}, TIPSY_COLOR_LIGHTGRAY, TIPSY_COLOR_BLACK, "BACK"};

#define SETTINGS_ITEM_CAPACITY (NUM_PUMPS + 1)
#define SETTINGS_LIST_X 18
#define SETTINGS_LIST_Y 72
#define SETTINGS_LIST_W 284
#define SETTINGS_LIST_H 300
#define SETTINGS_ROW_H 40
#define SETTINGS_ROW_GAP 8
#define SETTINGS_ROW_PITCH (SETTINGS_ROW_H + SETTINGS_ROW_GAP)
#define SETTINGS_SCROLLBAR_W 10
#define DRINKS_ITEM_CAPACITY 8
#define SHOTS_ITEM_CAPACITY (NUM_INGREDIENTS + 1)
#define MAPPING_ITEM_CAPACITY (NUM_INGREDIENTS + 1)

static uint16_t get_settings_content_height(size_t item_count) {
  if (item_count == 0) {
    return 0;
  }
  return (uint16_t)(item_count * SETTINGS_ROW_H +
                    (item_count - 1) * SETTINGS_ROW_GAP);
}

static uint16_t get_settings_scroll_max(size_t item_count) {
  const uint16_t content_h = get_settings_content_height(item_count);
  if (content_h <= SETTINGS_LIST_H) {
    return 0;
  }
  return (uint16_t)(content_h - SETTINGS_LIST_H);
}

static void render_generic_menu_list(const char *title, const char *hint_a,
                                     const char *hint_b, const MenuListItem *items,
                                     size_t item_count, uint16_t scroll_offset_px,
                                     uint16_t fill_color) {
  tipsy_display_fill_screen(TIPSY_COLOR_BLACK);
  draw_header(title);
  tipsy_display_fill_rect(SETTINGS_LIST_X, SETTINGS_LIST_Y, SETTINGS_LIST_W,
                          SETTINGS_LIST_H, TIPSY_COLOR_DARKGRAY);

  for (size_t i = 0; i < item_count; ++i) {
    const int32_t row_y =
        (int32_t)SETTINGS_LIST_Y + (int32_t)(i * SETTINGS_ROW_PITCH) -
        (int32_t)scroll_offset_px;
    const int32_t visible_top =
        row_y < SETTINGS_LIST_Y ? SETTINGS_LIST_Y : row_y;
    const int32_t visible_bottom =
        row_y + SETTINGS_ROW_H > SETTINGS_LIST_Y + SETTINGS_LIST_H
            ? SETTINGS_LIST_Y + SETTINGS_LIST_H
            : row_y + SETTINGS_ROW_H;
    const uint16_t row_fill = items[i].is_back ? TIPSY_COLOR_LIGHTGRAY : fill_color;
    const uint16_t row_text = items[i].is_back ? TIPSY_COLOR_BLACK : TIPSY_COLOR_WHITE;

    if (row_y + SETTINGS_ROW_H <= SETTINGS_LIST_Y ||
        row_y >= SETTINGS_LIST_Y + SETTINGS_LIST_H) {
      continue;
    }

    tipsy_display_fill_rect(SETTINGS_LIST_X, (uint16_t)visible_top,
                            SETTINGS_LIST_W - SETTINGS_SCROLLBAR_W - 6,
                            (uint16_t)(visible_bottom - visible_top), row_fill);
    if (row_y >= SETTINGS_LIST_Y &&
        row_y + SETTINGS_ROW_H <= SETTINGS_LIST_Y + SETTINGS_LIST_H) {
      tipsy_display_draw_text_5x7(SETTINGS_LIST_X + 8, (uint16_t)row_y + 14,
                                  items[i].label, row_text, 1);
    }
  }

  render_settings_scrollbar(item_count, scroll_offset_px);
  tipsy_display_draw_text_5x7(20, 388, hint_a, TIPSY_COLOR_WHITE, 1);
  tipsy_display_draw_text_5x7(20, 404, hint_b, TIPSY_COLOR_WHITE, 1);
}

static void draw_text_centered(const TipsyRect *rect, const char *text,
                               uint16_t color, uint8_t scale) {
  const size_t len = strlen(text);
  const uint16_t text_width = (uint16_t)(len * 6 * scale);
  const uint16_t text_height = 7 * scale;
  const uint16_t text_x = rect->x + (rect->w - text_width) / 2;
  const uint16_t text_y = rect->y + (rect->h - text_height) / 2;
  tipsy_display_draw_text_5x7(text_x, text_y, text, color, scale);
}

static void draw_button(const UiButton *button, bool highlighted) {
  const uint16_t fill = highlighted ? TIPSY_COLOR_WHITE : button->fill_color;
  const uint16_t text = highlighted ? TIPSY_COLOR_BLUE : button->text_color;
  tipsy_display_fill_rect(button->rect.x, button->rect.y, button->rect.w,
                          button->rect.h, fill);
  draw_text_centered(&button->rect, button->label, text, 3);
}

static void draw_header(const char *title) {
  tipsy_display_fill_rect(0, 0, TIPSY_LCD_WIDTH, 58, TIPSY_COLOR_LIGHTGRAY);
  tipsy_display_draw_text_5x7(12, 18, title, TIPSY_COLOR_BLACK, 2);
}

static void draw_footer(const char *status_line) {
  tipsy_display_fill_rect(0, 430, TIPSY_LCD_WIDTH, 50, TIPSY_COLOR_DARKGRAY);
  tipsy_display_draw_text_5x7(10, 446, status_line, TIPSY_COLOR_WHITE, 1);
}

static void render_boot_screen(const TipsyAppState *app_state) {
  const TipsyRect title = {20, 110, 280, 40};
  const TipsyRect subtitle = {32, 190, 256, 24};

  tipsy_display_fill_screen(TIPSY_COLOR_BLACK);
  draw_text_centered(&title, "Tipsy UI Refactor", TIPSY_COLOR_WHITE, 3);
  draw_text_centered(&subtitle, "Booting hardware", TIPSY_COLOR_LIGHTGRAY, 2);
  draw_footer(app_state->status_line);
}

static void render_main_menu_screen(const TipsyAppState *app_state) {
  tipsy_display_fill_screen(TIPSY_COLOR_DARKGRAY);
  draw_header("MAIN MENU");
  for (size_t i = 0; i < 3; ++i) {
    draw_button(&main_menu_buttons[i], false);
  }
  tipsy_display_draw_text_5x7(42, 390, "Refactor test path: SETTINGS",
                              TIPSY_COLOR_WHITE, 1);
  draw_footer(app_state->status_line);
}

static void render_settings_scrollbar(size_t item_count,
                                      uint16_t scroll_offset_px) {
  const uint16_t content_h = get_settings_content_height(item_count);
  const uint16_t scroll_max = get_settings_scroll_max(item_count);

  tipsy_display_fill_rect(SETTINGS_LIST_X + SETTINGS_LIST_W - SETTINGS_SCROLLBAR_W,
                          SETTINGS_LIST_Y, SETTINGS_SCROLLBAR_W, SETTINGS_LIST_H,
                          TIPSY_COLOR_BLACK);

  if (scroll_max == 0 || content_h == 0) {
    tipsy_display_fill_rect(
        SETTINGS_LIST_X + SETTINGS_LIST_W - SETTINGS_SCROLLBAR_W, SETTINGS_LIST_Y,
        SETTINGS_SCROLLBAR_W, SETTINGS_LIST_H, TIPSY_COLOR_GREEN);
    return;
  }

  {
    uint16_t thumb_h =
        (uint16_t)(((uint32_t)SETTINGS_LIST_H * SETTINGS_LIST_H) / content_h);
    uint16_t travel = 0;
    uint16_t thumb_y = SETTINGS_LIST_Y;

    if (thumb_h < 28) {
      thumb_h = 28;
    }
    if (thumb_h > SETTINGS_LIST_H) {
      thumb_h = SETTINGS_LIST_H;
    }

    travel = SETTINGS_LIST_H - thumb_h;
    thumb_y = SETTINGS_LIST_Y +
              (uint16_t)(((uint32_t)travel * scroll_offset_px) / scroll_max);

    tipsy_display_fill_rect(
        SETTINGS_LIST_X + SETTINGS_LIST_W - SETTINGS_SCROLLBAR_W, thumb_y,
        SETTINGS_SCROLLBAR_W, thumb_h, TIPSY_COLOR_GREEN);
  }
}

static void render_settings_screen(const TipsyAppState *app_state) {
  SettingsItem items[SETTINGS_ITEM_CAPACITY];
  const size_t item_count =
      tipsy_app_build_settings_items(items, SETTINGS_ITEM_CAPACITY);

  tipsy_display_fill_screen(TIPSY_COLOR_BLACK);
  draw_header("SETTINGS");
  tipsy_display_fill_rect(SETTINGS_LIST_X, SETTINGS_LIST_Y, SETTINGS_LIST_W,
                          SETTINGS_LIST_H, TIPSY_COLOR_DARKGRAY);

  for (size_t i = 0; i < item_count; ++i) {
    const int32_t row_y =
        (int32_t)SETTINGS_LIST_Y + (int32_t)(i * SETTINGS_ROW_PITCH) -
        (int32_t)app_state->settings_scroll_offset_px;
    const int32_t visible_top =
        row_y < SETTINGS_LIST_Y ? SETTINGS_LIST_Y : row_y;
    const int32_t visible_bottom =
        row_y + SETTINGS_ROW_H > SETTINGS_LIST_Y + SETTINGS_LIST_H
            ? SETTINGS_LIST_Y + SETTINGS_LIST_H
            : row_y + SETTINGS_ROW_H;
    const bool is_selected =
        items[i].type == SETTINGS_ITEM_PUMP &&
        items[i].pump_id == app_state->selected_settings_pump;
    const uint16_t fill = items[i].type == SETTINGS_ITEM_PUMP
                              ? TIPSY_COLOR_BLUE
                              : TIPSY_COLOR_LIGHTGRAY;
    const uint16_t text = items[i].type == SETTINGS_ITEM_PUMP
                              ? TIPSY_COLOR_WHITE
                              : TIPSY_COLOR_BLACK;
    const uint16_t accent = is_selected ? TIPSY_COLOR_GREEN : fill;

    if (row_y + SETTINGS_ROW_H <= SETTINGS_LIST_Y ||
        row_y >= SETTINGS_LIST_Y + SETTINGS_LIST_H) {
      continue;
    }

    tipsy_display_fill_rect(SETTINGS_LIST_X, (uint16_t)visible_top,
                            SETTINGS_LIST_W - SETTINGS_SCROLLBAR_W - 6,
                            (uint16_t)(visible_bottom - visible_top), accent);
    if (row_y >= SETTINGS_LIST_Y &&
        row_y + SETTINGS_ROW_H <= SETTINGS_LIST_Y + SETTINGS_LIST_H) {
      tipsy_display_draw_text_5x7(SETTINGS_LIST_X + 8, (uint16_t)row_y + 14,
                                  items[i].label, text, 1);
    }
  }

  render_settings_scrollbar(item_count, app_state->settings_scroll_offset_px);
  tipsy_display_draw_text_5x7(20, 388, "Drag list to scroll", TIPSY_COLOR_WHITE,
                              1);
  tipsy_display_draw_text_5x7(20, 404, "Tap pump to map, Back to return", TIPSY_COLOR_WHITE,
                              1);
  draw_footer(app_state->status_line);
}

static void render_drinks_screen(const TipsyAppState *app_state) {
  MenuListItem items[DRINKS_ITEM_CAPACITY];
  const size_t item_count =
      tipsy_app_build_drinks_items(items, DRINKS_ITEM_CAPACITY);

  render_generic_menu_list("DRINKS", "Tap item to open details",
                           "Back returns to main menu", items, item_count,
                           app_state->drinks_scroll_offset_px, TIPSY_COLOR_BLUE);
  draw_footer(app_state->status_line);
}

static void render_shots_screen(const TipsyAppState *app_state) {
  MenuListItem items[SHOTS_ITEM_CAPACITY];
  const size_t item_count =
      tipsy_app_build_shots_items(items, SHOTS_ITEM_CAPACITY);

  render_generic_menu_list("SHOTS", "Tap item to open details",
                           "Back returns to main menu", items, item_count,
                           app_state->shots_scroll_offset_px, TIPSY_COLOR_GREEN);
  draw_footer(app_state->status_line);
}

static void render_mapping_screen(const TipsyAppState *app_state) {
  MappingItem items[MAPPING_ITEM_CAPACITY];
  const size_t item_count =
      tipsy_app_build_mapping_items(items, MAPPING_ITEM_CAPACITY);

  tipsy_display_fill_screen(TIPSY_COLOR_BLACK);
  draw_header("MAPPING");
  tipsy_display_fill_rect(SETTINGS_LIST_X, SETTINGS_LIST_Y, SETTINGS_LIST_W,
                          SETTINGS_LIST_H, TIPSY_COLOR_DARKGRAY);
  tipsy_display_draw_text_5x7(20, 60, "Select ingredient", TIPSY_COLOR_WHITE, 1);

  for (size_t i = 0; i < item_count; ++i) {
    const int32_t row_y =
        (int32_t)SETTINGS_LIST_Y + (int32_t)(i * SETTINGS_ROW_PITCH) -
        (int32_t)app_state->mapping_scroll_offset_px;
    const int32_t visible_top =
        row_y < SETTINGS_LIST_Y ? SETTINGS_LIST_Y : row_y;
    const int32_t visible_bottom =
        row_y + SETTINGS_ROW_H > SETTINGS_LIST_Y + SETTINGS_LIST_H
            ? SETTINGS_LIST_Y + SETTINGS_LIST_H
            : row_y + SETTINGS_ROW_H;
    const bool is_selected =
        !items[i].is_back &&
        items[i].ingredient_id == tipsy_get_pumps()[app_state->selected_settings_pump].mapped_ingredient;
    const uint16_t fill = items[i].is_back ? TIPSY_COLOR_LIGHTGRAY : TIPSY_COLOR_GREEN;
    const uint16_t text = items[i].is_back ? TIPSY_COLOR_BLACK : TIPSY_COLOR_BLACK;
    const uint16_t accent = is_selected ? TIPSY_COLOR_BLUE : fill;

    if (row_y + SETTINGS_ROW_H <= SETTINGS_LIST_Y ||
        row_y >= SETTINGS_LIST_Y + SETTINGS_LIST_H) {
      continue;
    }

    tipsy_display_fill_rect(SETTINGS_LIST_X, (uint16_t)visible_top,
                            SETTINGS_LIST_W - SETTINGS_SCROLLBAR_W - 6,
                            (uint16_t)(visible_bottom - visible_top), accent);
    if (row_y >= SETTINGS_LIST_Y &&
        row_y + SETTINGS_ROW_H <= SETTINGS_LIST_Y + SETTINGS_LIST_H) {
      tipsy_display_draw_text_5x7(SETTINGS_LIST_X + 8, (uint16_t)row_y + 14,
                                  items[i].label,
                                  is_selected ? TIPSY_COLOR_WHITE : text, 1);
    }
  }

  render_settings_scrollbar(item_count, app_state->mapping_scroll_offset_px);
  tipsy_display_draw_text_5x7(20, 388, "Tap ingredient to assign", TIPSY_COLOR_WHITE,
                              1);
  tipsy_display_draw_text_5x7(20, 404, "Back returns to settings", TIPSY_COLOR_WHITE,
                              1);
  draw_footer(app_state->status_line);
}

static void render_detail_screen(const TipsyAppState *app_state) {
  const char *label = tipsy_app_get_selected_label(app_state);
  const uint16_t selected_ml = tipsy_app_get_selected_main_ml(app_state);

  tipsy_display_fill_screen(TIPSY_COLOR_BLACK);
  draw_header("DETAIL");
  tipsy_display_draw_text_5x7(20, 90, label, TIPSY_COLOR_WHITE, 3);
  tipsy_display_draw_text_5x7(20, 122, "Choose spirit ml", TIPSY_COLOR_LIGHTGRAY, 1);

  for (size_t i = 0; i < 3; ++i) {
    const uint16_t button_ml = (i == 0) ? 40 : ((i == 1) ? 60 : 80);
    draw_button(&detail_ml_buttons[i], selected_ml == button_ml);
  }

  draw_button(&detail_pour_button, false);
  draw_button(&detail_back_button, false);
  draw_footer(app_state->status_line);
}

static void draw_pouring_progress_bar(uint16_t fill_width) {
  const uint16_t bar_x = 20;
  const uint16_t bar_y = 220;
  const uint16_t bar_w = 280;
  const uint16_t bar_h = 40;

  tipsy_display_fill_rect(bar_x, bar_y, bar_w, bar_h, TIPSY_COLOR_LIGHTGRAY);
  if (fill_width > 0) {
    if (fill_width > bar_w) {
      fill_width = bar_w;
    }
    tipsy_display_fill_rect(bar_x, bar_y, fill_width, bar_h, TIPSY_COLOR_GREEN);
  }
}

static void render_pouring_screen(const TipsyAppState *app_state) {
  char ml_text[16];

  snprintf(ml_text, sizeof(ml_text), "%u ml",
           (unsigned int)tipsy_app_get_selected_main_ml(app_state));

  tipsy_display_fill_screen(TIPSY_COLOR_BLACK);
  draw_header("POURING");
  tipsy_display_draw_text_5x7(20, 95, tipsy_app_get_selected_label(app_state),
                              TIPSY_COLOR_WHITE, 3);
  tipsy_display_draw_text_5x7(20, 155, ml_text, TIPSY_COLOR_WHITE, 2);
  draw_pouring_progress_bar(tipsy_app_get_pour_progress_px(app_state));
  draw_footer(app_state->status_line);
}

void tipsy_ui_refactor_render(const TipsyAppState *app_state) {
  if (app_state == NULL) {
    return;
  }

  if (app_state->current_screen == UI_REFACTOR_SCREEN_BOOT) {
    render_boot_screen(app_state);
  } else if (app_state->current_screen == UI_REFACTOR_SCREEN_MAIN_MENU) {
    render_main_menu_screen(app_state);
  } else if (app_state->current_screen == UI_REFACTOR_SCREEN_DRINKS_LIST) {
    render_drinks_screen(app_state);
  } else if (app_state->current_screen == UI_REFACTOR_SCREEN_SHOTS_LIST) {
    render_shots_screen(app_state);
  } else if (app_state->current_screen == UI_REFACTOR_SCREEN_SETTINGS_LIST) {
    render_settings_screen(app_state);
  } else if (app_state->current_screen == UI_REFACTOR_SCREEN_SETTINGS_MAPPING) {
    render_mapping_screen(app_state);
  } else if (app_state->current_screen == UI_REFACTOR_SCREEN_DETAIL) {
    render_detail_screen(app_state);
  } else {
    render_pouring_screen(app_state);
  }

  tipsy_display_present();
}

static int get_main_menu_hit(uint16_t x, uint16_t y) {
  for (int i = 0; i < 3; ++i) {
    if (tipsy_rect_contains(&main_menu_buttons[i].rect, x, y)) {
      return i;
    }
  }
  return -1;
}

bool tipsy_ui_refactor_is_scroll_list_point(const TipsyAppState *app_state,
                                            uint16_t x, uint16_t y) {
  const TipsyRect list_rect = {SETTINGS_LIST_X, SETTINGS_LIST_Y, SETTINGS_LIST_W,
                               SETTINGS_LIST_H};
  if (app_state == NULL) {
    return false;
  }
  if (app_state->current_screen != UI_REFACTOR_SCREEN_SETTINGS_LIST &&
      app_state->current_screen != UI_REFACTOR_SCREEN_DRINKS_LIST &&
      app_state->current_screen != UI_REFACTOR_SCREEN_SHOTS_LIST &&
      app_state->current_screen != UI_REFACTOR_SCREEN_SETTINGS_MAPPING) {
    return false;
  }
  return tipsy_rect_contains(&list_rect, x, y);
}

bool tipsy_ui_refactor_scroll_active_list(TipsyAppState *app_state, int16_t delta_y) {
  size_t item_count = 0;
  int32_t next_offset = 0;
  uint16_t *offset_px = NULL;

  if (app_state == NULL || delta_y == 0) {
    return false;
  }

  if (app_state->current_screen == UI_REFACTOR_SCREEN_SETTINGS_LIST) {
    SettingsItem items[SETTINGS_ITEM_CAPACITY];
    item_count = tipsy_app_build_settings_items(items, SETTINGS_ITEM_CAPACITY);
    offset_px = &app_state->settings_scroll_offset_px;
  } else if (app_state->current_screen == UI_REFACTOR_SCREEN_DRINKS_LIST) {
    MenuListItem items[DRINKS_ITEM_CAPACITY];
    item_count = tipsy_app_build_drinks_items(items, DRINKS_ITEM_CAPACITY);
    offset_px = &app_state->drinks_scroll_offset_px;
  } else if (app_state->current_screen == UI_REFACTOR_SCREEN_SHOTS_LIST) {
    MenuListItem items[SHOTS_ITEM_CAPACITY];
    item_count = tipsy_app_build_shots_items(items, SHOTS_ITEM_CAPACITY);
    offset_px = &app_state->shots_scroll_offset_px;
  } else if (app_state->current_screen == UI_REFACTOR_SCREEN_SETTINGS_MAPPING) {
    MappingItem items[MAPPING_ITEM_CAPACITY];
    item_count = tipsy_app_build_mapping_items(items, MAPPING_ITEM_CAPACITY);
    offset_px = &app_state->mapping_scroll_offset_px;
  } else {
    return false;
  }

  {
    const uint16_t max_scroll = get_settings_scroll_max(item_count);
    if (max_scroll == 0) {
      return false;
    }

    next_offset = (int32_t)(*offset_px) - delta_y;
    if (next_offset < 0) {
      next_offset = 0;
    }
    if (next_offset > max_scroll) {
      next_offset = max_scroll;
    }
    if ((uint16_t)next_offset == *offset_px) {
      return false;
    }

    *offset_px = (uint16_t)next_offset;
    snprintf(app_state->status_line, sizeof(app_state->status_line),
             "List scroll %u/%u",
             (unsigned int)(*offset_px),
             (unsigned int)max_scroll);
    return true;
  }
}

bool tipsy_ui_refactor_handle_release(TipsyAppState *app_state, uint16_t x,
                                      uint16_t y) {
  int hit_index = -1;

  if (app_state == NULL) {
    return false;
  }

  if (app_state->current_screen == UI_REFACTOR_SCREEN_MAIN_MENU) {
    hit_index = get_main_menu_hit(x, y);
    if (hit_index >= 0) {
      tipsy_app_activate_main_menu(app_state, (uint8_t)hit_index);
      return true;
    }
    return false;
  }

  if (app_state->current_screen == UI_REFACTOR_SCREEN_DETAIL) {
    for (int i = 0; i < 3; ++i) {
      if (tipsy_rect_contains(&detail_ml_buttons[i].rect, x, y)) {
        tipsy_app_select_ml(app_state, (i == 0) ? 40 : ((i == 1) ? 60 : 80));
        return true;
      }
    }
    if (tipsy_rect_contains(&detail_pour_button.rect, x, y)) {
      tipsy_app_start_pour(app_state,
                           (uint32_t)to_ms_since_boot(get_absolute_time()));
      return true;
    }
    if (tipsy_rect_contains(&detail_back_button.rect, x, y)) {
      tipsy_app_back_from_detail(app_state);
      return true;
    }
    return false;
  }

  if (app_state->current_screen == UI_REFACTOR_SCREEN_POURING) {
    return false;
  }

  if (app_state->current_screen == UI_REFACTOR_SCREEN_DRINKS_LIST) {
    MenuListItem items[DRINKS_ITEM_CAPACITY];
    const size_t item_count =
        tipsy_app_build_drinks_items(items, DRINKS_ITEM_CAPACITY);
    uint32_t content_y = 0;

    if (!tipsy_ui_refactor_is_scroll_list_point(app_state, x, y)) {
      return false;
    }

    content_y = (uint32_t)app_state->drinks_scroll_offset_px +
                (uint32_t)(y - SETTINGS_LIST_Y);
    for (size_t i = 0; i < item_count; ++i) {
      const uint32_t row_start = i * SETTINGS_ROW_PITCH;
      const uint32_t row_end = row_start + SETTINGS_ROW_H;
      if (content_y >= row_start && content_y < row_end &&
          x < SETTINGS_LIST_X + SETTINGS_LIST_W - SETTINGS_SCROLLBAR_W - 6) {
        hit_index = (int)i;
        break;
      }
    }

    if (hit_index >= 0) {
      tipsy_app_activate_drinks_item(app_state, (size_t)hit_index);
      return true;
    }
  } else if (app_state->current_screen == UI_REFACTOR_SCREEN_SHOTS_LIST) {
    MenuListItem items[SHOTS_ITEM_CAPACITY];
    const size_t item_count =
        tipsy_app_build_shots_items(items, SHOTS_ITEM_CAPACITY);
    uint32_t content_y = 0;

    if (!tipsy_ui_refactor_is_scroll_list_point(app_state, x, y)) {
      return false;
    }

    content_y = (uint32_t)app_state->shots_scroll_offset_px +
                (uint32_t)(y - SETTINGS_LIST_Y);
    for (size_t i = 0; i < item_count; ++i) {
      const uint32_t row_start = i * SETTINGS_ROW_PITCH;
      const uint32_t row_end = row_start + SETTINGS_ROW_H;
      if (content_y >= row_start && content_y < row_end &&
          x < SETTINGS_LIST_X + SETTINGS_LIST_W - SETTINGS_SCROLLBAR_W - 6) {
        hit_index = (int)i;
        break;
      }
    }

    if (hit_index >= 0) {
      tipsy_app_activate_shots_item(app_state, (size_t)hit_index);
      return true;
    }
  }

  if (app_state->current_screen == UI_REFACTOR_SCREEN_SETTINGS_LIST) {
    SettingsItem items[SETTINGS_ITEM_CAPACITY];
    const size_t item_count =
        tipsy_app_build_settings_items(items, SETTINGS_ITEM_CAPACITY);
    uint32_t content_y = 0;

    if (!tipsy_ui_refactor_is_scroll_list_point(app_state, x, y)) {
      return false;
    }

    content_y = (uint32_t)app_state->settings_scroll_offset_px +
                (uint32_t)(y - SETTINGS_LIST_Y);
    for (size_t i = 0; i < item_count; ++i) {
      const uint32_t row_start = i * SETTINGS_ROW_PITCH;
      const uint32_t row_end = row_start + SETTINGS_ROW_H;
      if (content_y >= row_start && content_y < row_end &&
          x < SETTINGS_LIST_X + SETTINGS_LIST_W - SETTINGS_SCROLLBAR_W - 6) {
        hit_index = (int)i;
        break;
      }
    }

    if (hit_index >= 0) {
      tipsy_app_activate_settings_item(app_state, (size_t)hit_index);
      return true;
    }
  } else if (app_state->current_screen == UI_REFACTOR_SCREEN_SETTINGS_MAPPING) {
    MappingItem items[MAPPING_ITEM_CAPACITY];
    const size_t item_count =
        tipsy_app_build_mapping_items(items, MAPPING_ITEM_CAPACITY);
    uint32_t content_y = 0;

    if (!tipsy_ui_refactor_is_scroll_list_point(app_state, x, y)) {
      return false;
    }

    content_y = (uint32_t)app_state->mapping_scroll_offset_px +
                (uint32_t)(y - SETTINGS_LIST_Y);
    for (size_t i = 0; i < item_count; ++i) {
      const uint32_t row_start = i * SETTINGS_ROW_PITCH;
      const uint32_t row_end = row_start + SETTINGS_ROW_H;
      if (content_y >= row_start && content_y < row_end &&
          x < SETTINGS_LIST_X + SETTINGS_LIST_W - SETTINGS_SCROLLBAR_W - 6) {
        hit_index = (int)i;
        break;
      }
    }

    if (hit_index >= 0) {
      tipsy_app_activate_mapping_item(app_state, (size_t)hit_index);
      return true;
    }
  }

  return false;
}
