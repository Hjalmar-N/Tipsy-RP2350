#ifndef TIPSY_UI_REFACTOR_H
#define TIPSY_UI_REFACTOR_H

#include <stdbool.h>
#include <stdint.h>

#include "tipsy_app.h"

void tipsy_ui_refactor_render(const TipsyAppState *app_state);
bool tipsy_ui_refactor_handle_release(TipsyAppState *app_state, uint16_t x,
                                      uint16_t y);
bool tipsy_ui_refactor_is_scroll_list_point(const TipsyAppState *app_state,
                                            uint16_t x, uint16_t y);
bool tipsy_ui_refactor_scroll_active_list(TipsyAppState *app_state, int16_t delta_y);

#endif
