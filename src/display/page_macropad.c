/*
 * SPDX-License-Identifier: MIT
 *
 * Macropad screen: two large touch buttons that emit INPUT_KEY events,
 * plus a back button to return to the home screen.
 *
 * Layout (circle r=120, safe area x∈[36,204] y∈[36,204]):
 *
 *   y=50  [      Button 1  x=36 w=168 h=50     ]
 *   y=110 [      Button 2  x=36 w=168 h=50     ]
 *   y=175 [    ◁ Back  x=72 w=96  h=36   ]
 */

#include <lvgl.h>
#include "page_ops.h"

/* ── Button styles ─────────────────────────────────────────────────────── */

static lv_style_t s_style_normal;
static lv_style_t s_style_pressed;
static bool s_styles_init;

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

/* ── Button callbacks ──────────────────────────────────────────────────── */

static void cb_key_btn(lv_event_t *e)
{
	lv_obj_t *btn = lv_event_get_target(e);
	lv_event_code_t ev = lv_event_get_code(e);
	enum ss_key_code key = (enum ss_key_code)(uintptr_t)lv_event_get_user_data(e);

	if (ev == LV_EVENT_PRESSED) {
		ss_send_key(key, true);
		lv_obj_add_style(btn, &s_style_pressed, LV_PART_MAIN);
	} else if (ev == LV_EVENT_RELEASED) {
		ss_send_key(key, false);
		lv_obj_remove_style(btn, &s_style_pressed, LV_PART_MAIN);
	}
}

static void cb_back_btn(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		ss_navigate_to(PAGE_HOME);
	}
}

/* ── Button factories ──────────────────────────────────────────────────── */

static lv_obj_t *make_btn(lv_obj_t *parent, const char *text,
			   int x, int y, int w, int h,
			   enum ss_key_code key)
{
	lv_obj_t *btn = lv_obj_create(parent);
	lv_obj_set_size(btn, w, h);
	lv_obj_set_pos(btn, x, y);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_style(btn, &s_style_normal, LV_PART_MAIN);
	lv_obj_add_event_cb(btn, cb_key_btn, LV_EVENT_ALL,
			    (void *)(uintptr_t)key);

	lv_obj_t *lbl = lv_label_create(btn);
	lv_label_set_text(lbl, text);
	lv_obj_center(lbl);

	return btn;
}

static lv_obj_t *make_back_btn(lv_obj_t *parent, int x, int y, int w, int h)
{
	lv_obj_t *btn = lv_obj_create(parent);
	lv_obj_set_size(btn, w, h);
	lv_obj_set_pos(btn, x, y);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_style(btn, &s_style_normal, LV_PART_MAIN);
	lv_obj_add_event_cb(btn, cb_back_btn, LV_EVENT_ALL, NULL);

	lv_obj_t *lbl = lv_label_create(btn);
	lv_label_set_text(lbl, LV_SYMBOL_LEFT " Back");
	lv_obj_center(lbl);

	return btn;
}

/* ── Page interface ────────────────────────────────────────────────────── */

static int page_macropad_create(lv_obj_t *screen)
{
	init_styles();

	make_btn(screen,      "Button 1", 36,  50, 168, 50, SS_KEY_1);
	make_btn(screen,      "Button 2", 36, 110, 168, 50, SS_KEY_2);
	make_back_btn(screen,             72, 175,  96, 36);

	return 0;
}

const struct page_ops page_macropad_ops = {
	.name         = "macropad",
	.mouse_active = false,
	.create       = page_macropad_create,
	.on_enter     = NULL,
	.on_leave     = NULL,
};
