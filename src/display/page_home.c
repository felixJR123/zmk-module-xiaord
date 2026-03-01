/*
 * SPDX-License-Identifier: MIT
 *
 * Home screen: full-screen mouse surface with a menu button at the bottom.
 * Background touches are routed to the virtual pointer by the coordinator.
 * Long-pressing the menu button navigates to the macropad screen.
 */

#include <lvgl.h>
#include "page_ops.h"

static void menu_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED) {
		ss_navigate_to(PAGE_CLOCK);
	}
}

static int page_home_create(lv_obj_t *tile)
{
	/* Menu button — bottom-center, small enough to stay in the safe area */
	lv_obj_t *btn = lv_obj_create(tile);
	lv_obj_set_size(btn, 48, 48);
	lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -16);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(btn, menu_btn_cb, LV_EVENT_ALL, NULL);

	lv_obj_t *lbl = lv_label_create(btn);
	lv_label_set_text(lbl, LV_SYMBOL_LIST);
	lv_obj_center(lbl);

	return 0;
}

const struct page_ops page_home_ops = {
	.name         = "home",
	.mouse_active = true,
	.create       = page_home_create,
	.on_enter     = NULL,
	.on_leave     = NULL,
};
