#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"

#include "tipsy_app.h"
#include "tipsy_hal.h"
#include "tipsy_ui_refactor.h"

int main(void) {
  TipsyAppState app_state;
  TipsyTouchSample touch_sample;
  bool last_pressed = false;
  bool touch_started_in_settings_list = false;
  bool touch_scrolled = false;
  uint16_t last_touch_x = 0;
  uint16_t last_touch_y = 0;

  tipsy_hal_init_stdio_and_led();

  printf("\r\n=== Tipsy-RP2350 UI Refactor Prototype ===\r\n");
  fflush(stdout);

  tipsy_display_init();
  tipsy_touch_init();

  tipsy_app_init(&app_state);
  tipsy_ui_refactor_render(&app_state);

  sleep_ms(450);
  tipsy_app_finish_boot(&app_state);
  tipsy_ui_refactor_render(&app_state);

  while (true) {
    const bool pressed_now = tipsy_touch_read(&touch_sample);

    if (pressed_now) {
      if (!last_pressed) {
        touch_scrolled = false;
        touch_started_in_settings_list =
            tipsy_ui_refactor_is_scroll_list_point(&app_state, touch_sample.x,
                                                   touch_sample.y);
      } else if (touch_started_in_settings_list) {
        const int16_t delta_y = (int16_t)touch_sample.y - (int16_t)last_touch_y;
        if ((delta_y >= 14 || delta_y <= -14) &&
            tipsy_ui_refactor_scroll_active_list(&app_state, delta_y)) {
          touch_scrolled = true;
          tipsy_ui_refactor_render(&app_state);
        }
      }

      last_touch_x = touch_sample.x;
      last_touch_y = touch_sample.y;
    }

    if (!pressed_now && last_pressed) {
      if (!touch_scrolled &&
          tipsy_ui_refactor_handle_release(&app_state, last_touch_x,
                                           last_touch_y)) {
        tipsy_ui_refactor_render(&app_state);
        printf("UI action handled, screen=%d status=%s\r\n",
               (int)app_state.current_screen, app_state.status_line);
        fflush(stdout);
      }
      touch_started_in_settings_list = false;
      touch_scrolled = false;
    }

    last_pressed = pressed_now;
    sleep_ms(15);
  }

  return 0;
}
