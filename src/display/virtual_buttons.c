/*
 * SPDX-License-Identifier: MIT
 *
 * Virtual touch button screen for the xiaord dongle 240×240 display.
 * Button taps report INPUT_KEY events via Zephyr input API.
 *
 * Layout (circle r=120, safe area x∈[36,204] y∈[36,204]):
 *
 *   y=60  [      Button 1  x=36 w=168 h=60     ]
 *   y=130 [      Button 2  x=36 w=168 h=60     ]
 *   y=185 [      Back      x=38 w=164 h=38     ]
 */

#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <lvgl.h>
#include "virtual_buttons.h"

static lv_obj_t *s_status_screen;

/* ── Button styles ─────────────────────────────────────────────────────── */

static lv_style_t s_style_normal;
static lv_style_t s_style_pressed;
static bool s_styles_init = false;

static void init_styles(void)
{
	if (s_styles_init) {
		return;
	}
	s_styles_init = true;

	lv_style_init(&s_style_normal);
	lv_style_set_bg_color(&s_style_normal, lv_palette_main(LV_PALETTE_BLUE_GREY));
	lv_style_set_bg_opa(&s_style_normal, LV_OPA_COVER);
	lv_style_set_border_color(&s_style_normal, lv_color_white());
	lv_style_set_border_width(&s_style_normal, 2);
	lv_style_set_radius(&s_style_normal, 8);

	lv_style_init(&s_style_pressed);
	lv_style_set_bg_color(&s_style_pressed, lv_palette_main(LV_PALETTE_GREEN));
	lv_style_set_bg_opa(&s_style_pressed, LV_OPA_COVER);
}

/* ── Button callbacks (input_report_key + visual feedback) ─────────────── */

static void cb_key_btn(lv_event_t *e)
{
	lv_obj_t *btn = lv_event_get_target(e);
	lv_event_code_t ev = lv_event_get_code(e);
	uint16_t key = (uint16_t)(uintptr_t)lv_event_get_user_data(e);

	if (ev == LV_EVENT_PRESSED) {
		input_report_key(NULL, key, 1, true, K_NO_WAIT);
		lv_obj_add_style(btn, &s_style_pressed, LV_PART_MAIN);
	} else if (ev == LV_EVENT_RELEASED) {
		input_report_key(NULL, key, 0, true, K_NO_WAIT);
		lv_obj_remove_style(btn, &s_style_pressed, LV_PART_MAIN);
	}
}

static void cb_back(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		lv_scr_load(s_status_screen);
	}
}

/* ── Button factory ────────────────────────────────────────────────────── */

static lv_obj_t *make_btn(lv_obj_t *parent, const char *text,
			   int x, int y, int w, int h,
			   lv_event_cb_t cb, void *user_data)
{
	lv_obj_t *btn = lv_obj_create(parent);
	lv_obj_set_size(btn, w, h);
	lv_obj_set_pos(btn, x, y);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_style(btn, &s_style_normal, LV_PART_MAIN);
	lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, user_data);

	lv_obj_t *lbl = lv_label_create(btn);
	lv_label_set_text(lbl, text);
	lv_obj_center(lbl);

	return btn;
}

/* ── Public API ────────────────────────────────────────────────────────── */

int zmk_widget_virtual_buttons_init(struct zmk_widget_virtual_buttons *widget,
                                    lv_obj_t *return_screen)
{
	s_status_screen = return_screen;

	init_styles();

	widget->obj = lv_obj_create(NULL);

	/* Two large buttons — report INPUT_KEY_1 / INPUT_KEY_2 */
	make_btn(widget->obj, "Button 1", 36,  60, 168, 60,
		 cb_key_btn, (void *)(uintptr_t)INPUT_KEY_1);
	make_btn(widget->obj, "Button 2", 36, 130, 168, 60,
		 cb_key_btn, (void *)(uintptr_t)INPUT_KEY_2);

	/* Back to status screen */
	make_btn(widget->obj, LV_SYMBOL_LEFT " Back", 38, 185, 164, 38,
		 cb_back, NULL);

	return 0;
}

lv_obj_t *zmk_widget_virtual_buttons_obj(struct zmk_widget_virtual_buttons *widget)
{
	return widget->obj;
}
