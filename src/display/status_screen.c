/*
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <lvgl.h>

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BATTERY_STATUS)
#include <zmk/display/widgets/battery_status.h>
static struct zmk_widget_battery_status battery_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
#include <zmk/display/widgets/output_status.h>
static struct zmk_widget_output_status output_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
#include <zmk/display/widgets/layer_status.h>
static struct zmk_widget_layer_status layer_widget;
#endif


/* Margin to keep widgets inside the visible area of the round 240×240 display */
#define ROUND_MARGIN 60

#if IS_ENABLED(CONFIG_XIAORD_VIRTUAL_BUTTONS)
#include "virtual_buttons.h"
static struct zmk_widget_virtual_buttons virtual_buttons_widget;
static lv_obj_t *s_button_screen;
#endif

static inline void xiaord_initialize_color_theme(void) {
#if IS_ENABLED(CONFIG_LV_USE_THEME_DEFAULT)
    lv_display_t *disp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(
        disp,
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_TEAL),
        true,                    /* dark mode */
        &lv_font_montserrat_16   /* default font */
    );
    if (theme) {
        lv_display_set_theme(disp, theme);
    }
#endif
}



lv_obj_t *zmk_display_status_screen() {
    /* Initialize color theme — ZMK's initialize_theme() only handles monochrome */
    xiaord_initialize_color_theme();
    lv_obj_t *screen = lv_obj_create(NULL);

    /* Test: solid red background to verify LVGL rendering pipeline */
    // lv_obj_set_style_bg_color(screen, lv_color_hex(0xFF0000), LV_PART_MAIN);
    // lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

// #if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
//     zmk_widget_output_status_init(&output_widget, screen);
//     lv_obj_align(zmk_widget_output_status_obj(&output_widget),
//                  LV_ALIGN_TOP_LEFT, ROUND_MARGIN, ROUND_MARGIN);
// #endif

// #if IS_ENABLED(CONFIG_ZMK_WIDGET_BATTERY_STATUS)
//     zmk_widget_battery_status_init(&battery_widget, screen);
//     lv_obj_align(zmk_widget_battery_status_obj(&battery_widget),
//                  LV_ALIGN_TOP_RIGHT, -ROUND_MARGIN, ROUND_MARGIN);
// #endif

// #if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
//     zmk_widget_layer_status_init(&layer_widget, screen);
//     lv_obj_t *layer_obj = zmk_widget_layer_status_obj(&layer_widget);
//     lv_obj_align(layer_obj, LV_ALIGN_CENTER, 0, 0);
// #endif


#if IS_ENABLED(CONFIG_XIAORD_VIRTUAL_BUTTONS)
	zmk_widget_virtual_buttons_init(&virtual_buttons_widget, screen);
	s_button_screen = zmk_widget_virtual_buttons_obj(&virtual_buttons_widget);
	return s_button_screen;
#else
	return screen;
#endif
}