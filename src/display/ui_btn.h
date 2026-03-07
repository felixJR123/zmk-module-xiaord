/* SPDX-License-Identifier: MIT */
#pragma once
#include <lvgl.h>

#define UI_CIRCLE_BTN_DIAMETER  44
#define UI_CIRCLE_LAYOUT_RADIUS (120 - UI_CIRCLE_BTN_DIAMETER / 2 + 7)

/* Fill out[0..11][0..1] with (x, y) offsets for 12 clock positions.
 * Slot 0 = 12 o'clock, clockwise in 30° steps. */
void ui_circle_12_positions(int16_t (*out)[2], int16_t radius);

lv_obj_t *ui_create_btn(lv_obj_t *parent, const char *text,
			 lv_align_t align, int16_t x_off, int16_t y_off,
			 int16_t w, int16_t h, int32_t radius,
			 lv_event_cb_t cb, void *user_data);

lv_obj_t *ui_create_circle_btn(lv_obj_t *parent, const char *text,
				int16_t x_off, int16_t y_off,
				lv_event_cb_t cb, void *user_data);

lv_obj_t *ui_create_action_btn(lv_obj_t *parent, const char *text,
				int16_t x_off, int16_t y_off,
				lv_event_cb_t cb, void *user_data);
