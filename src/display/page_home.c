/*
 * SPDX-License-Identifier: MIT
 *
 * Home screen: full-screen view with a menu button at the bottom.
 * Long-pressing the menu button navigates to the macropad screen.
 * Long-pressing the bootloader button enters DFU/bootloader mode.
 */

#include <lvgl.h>
#include <zmk/split/central.h>
#include <dt-bindings/xiaord/input_codes.h>
#include "page_ops.h"
#include "home_status.h"

static void menu_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED) {
		ss_navigate_to(PAGE_CLOCK);
	}
}

static void bootloader_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED) {
		ss_fire_behavior(ZMK_BEHAVIOR_BOOTLOADER);
	}
}

static int page_home_create(lv_obj_t *tile)
{
	/* Output endpoint status — top area, centered */
	lv_obj_t *output_lbl = lv_label_create(tile);
	lv_label_set_text(output_lbl, "...");
	lv_obj_align(output_lbl, LV_ALIGN_TOP_MID, 0, 55);

	/* Peripheral battery labels — one per peripheral, stacked below output */
	lv_obj_t *periph_bat_lbls[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];

	for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; i++) {
		periph_bat_lbls[i] = lv_label_create(tile);
		lv_label_set_text_fmt(periph_bat_lbls[i], "P%d: --", i);
		lv_obj_align(periph_bat_lbls[i], LV_ALIGN_TOP_MID, 0, 90 + i * 35);
	}

	/* Wire up event-driven updates */
	home_status_init(output_lbl, periph_bat_lbls);

	/* Menu button — bottom-center-left */
	lv_obj_t *btn = lv_obj_create(tile);
	lv_obj_set_size(btn, 48, 48);
	lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, -30, -16);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(btn, menu_btn_cb, LV_EVENT_ALL, NULL);

	lv_obj_t *lbl = lv_label_create(btn);
	lv_label_set_text(lbl, LV_SYMBOL_LIST);
	lv_obj_center(lbl);

	/* Bootloader button — bottom-center-right, long-press to enter DFU */
	lv_obj_t *boot_btn = lv_obj_create(tile);
	lv_obj_set_size(boot_btn, 48, 48);
	lv_obj_align(boot_btn, LV_ALIGN_BOTTOM_MID, 30, -16);
	lv_obj_add_flag(boot_btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(boot_btn, bootloader_btn_cb, LV_EVENT_ALL, NULL);

	lv_obj_t *boot_lbl = lv_label_create(boot_btn);
	lv_label_set_text(boot_lbl, LV_SYMBOL_UPLOAD);
	lv_obj_center(boot_lbl);

	return 0;
}

const struct page_ops page_home_ops = {
	.name         = "home",
	.create       = page_home_create,
	.on_enter     = NULL,
	.on_leave     = NULL,
};
