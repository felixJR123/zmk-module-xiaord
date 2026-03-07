/* SPDX-License-Identifier: MIT */
#include "ui_btn.h"

static lv_style_t s_style_base;
static lv_style_t s_style_pressed;
static bool       s_init;

static void ensure_init(void)
{
	if (s_init) {
		return;
	}
	s_init = true;

	lv_style_init(&s_style_base);
	lv_style_set_bg_color(&s_style_base, lv_color_white());
	lv_style_set_bg_opa(&s_style_base, LV_OPA_50);
	lv_style_set_border_width(&s_style_base, 0);

	lv_style_init(&s_style_pressed);
	lv_style_set_bg_color(&s_style_pressed, lv_palette_main(LV_PALETTE_BLUE));
	lv_style_set_bg_opa(&s_style_pressed, LV_OPA_50);
}

lv_obj_t *ui_create_btn(lv_obj_t *parent, const char *text,
			 lv_align_t align, int16_t x_off, int16_t y_off,
			 int16_t w, int16_t h, int32_t radius,
			 lv_event_cb_t cb, void *user_data)
{
	ensure_init();

	lv_obj_t *btn = lv_obj_create(parent);

	lv_obj_set_size(btn, w, h);
	lv_obj_align(btn, align, x_off, y_off);
	lv_obj_set_style_radius(btn, radius, 0);
	lv_obj_add_style(btn, &s_style_base,    LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_add_style(btn, &s_style_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, user_data);

	lv_obj_t *lbl = lv_label_create(btn);

	lv_label_set_text(lbl, text);
	lv_obj_center(lbl);

	return btn;
}

lv_obj_t *ui_create_circle_btn(lv_obj_t *parent, const char *text,
				int16_t x_off, int16_t y_off,
				lv_event_cb_t cb, void *user_data)
{
	return ui_create_btn(parent, text, LV_ALIGN_CENTER, x_off, y_off,
			     44, 44, LV_RADIUS_CIRCLE, cb, user_data);
}
