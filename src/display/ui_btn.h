/* SPDX-License-Identifier: MIT */
#pragma once
#include <lvgl.h>

lv_obj_t *ui_create_btn(lv_obj_t *parent, const char *text,
			 lv_align_t align, int16_t x_off, int16_t y_off,
			 int16_t w, int16_t h, int32_t radius,
			 lv_event_cb_t cb, void *user_data);

lv_obj_t *ui_create_circle_btn(lv_obj_t *parent, const char *text,
				int16_t x_off, int16_t y_off,
				lv_event_cb_t cb, void *user_data);
