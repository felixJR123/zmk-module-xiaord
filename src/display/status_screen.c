/*
 * SPDX-License-Identifier: MIT
 *
 * Xiaord display coordinator — independent-screen multi-page system.
 *
 * Responsibilities:
 *  - Create independent LVGL screens, register all pages
 *  - Route touch events: mouse_active pages → virtual pointer, others → native LVGL
 *  - Manage page lifecycle (on_enter / on_leave) on transitions
 *  - Provide ss_navigate_to() and ss_send_key() APIs for pages to call
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <lvgl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <virtual_pointer.h>
#include "page_ops.h"

BUILD_ASSERT(IS_ENABLED(CONFIG_ZMK_VIRTUAL_POINTER),
	"xiaord status_screen requires CONFIG_ZMK_VIRTUAL_POINTER");
BUILD_ASSERT(IS_ENABLED(CONFIG_LV_USE_THEME_DEFAULT),
	"xiaord status_screen requires CONFIG_LV_USE_THEME_DEFAULT");

/* ── Page declarations ─────────────────────────────────────────────────── */

extern const struct page_ops page_home_ops;
extern const struct page_ops page_clock_ops;
extern const struct page_ops page_macropad_ops;

/* ── Virtual pointer device ────────────────────────────────────────────── */

static const struct device *s_vpointer = DEVICE_DT_GET(DT_NODELABEL(vpointer));

/* ── Page registration table ───────────────────────────────────────────── */

struct page_entry {
	const struct page_ops *ops;
	lv_obj_t *screen; /* independent screen created with lv_obj_create(NULL) */
};

static struct page_entry s_pages[] = {
	[PAGE_HOME]     = { .ops = &page_home_ops },
	[PAGE_CLOCK]    = { .ops = &page_clock_ops },
	[PAGE_MACROPAD] = { .ops = &page_macropad_ops },
};

#define PAGE_COUNT ARRAY_SIZE(s_pages)

/* ── Active page tracking ──────────────────────────────────────────────── */

static uint8_t s_active_page;

/* ── Key map ────────────────────────────────────────────────────────────── */

static const uint16_t s_key_map[SS_KEY_COUNT] = {
	[SS_KEY_1] = INPUT_KEY_1,
	[SS_KEY_2] = INPUT_KEY_2,
};

/* ── Public API ─────────────────────────────────────────────────────────── */

void ss_navigate_to(uint8_t page_idx)
{
	if (page_idx >= PAGE_COUNT || page_idx == s_active_page) {
		return;
	}

	/* Leave current page */
	struct page_entry *old = &s_pages[s_active_page];
	if (old->ops->mouse_active) {
		zmk_virtual_pointer_feed(s_vpointer, 0, 0, false);
	}
	if (old->ops->on_leave) {
		old->ops->on_leave();
	}

	/* Enter new page */
	s_active_page = page_idx;
	lv_scr_load(s_pages[page_idx].screen);
	if (s_pages[page_idx].ops->on_enter) {
		s_pages[page_idx].ops->on_enter();
	}
}

void ss_send_key(enum ss_key_code key, bool pressed)
{
	if (key <= 0 || key >= SS_KEY_COUNT) {
		LOG_WRN("ss_send_key: invalid key %d", key);
		return;
	}
	input_report_key(s_vpointer, s_key_map[key], pressed ? 1 : 0, true, K_NO_WAIT);
}

/* ── Touch routing callback ─────────────────────────────────────────────── */

static void tile_touch_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_indev_t *indev = lv_indev_get_act();
	lv_point_t pt;

	lv_indev_get_point(indev, &pt);
	bool pressed = (code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST);

	zmk_virtual_pointer_feed(s_vpointer, (int16_t)pt.x, (int16_t)pt.y, pressed);
}

/* ── Color theme ─────────────────────────────────────────────────────────── */

static void xiaord_initialize_color_theme(void)
{
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
}

/* ── Entry point called by ZMK display subsystem ───────────────────────── */

lv_obj_t *zmk_display_status_screen(void)
{
	xiaord_initialize_color_theme();

	/* Create an independent screen for each page */
	for (size_t i = 0; i < PAGE_COUNT; i++) {
		lv_obj_t *screen = lv_obj_create(NULL);
		lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
		s_pages[i].screen = screen;

		/* Build page widgets */
		if (s_pages[i].ops->create) {
			s_pages[i].ops->create(screen);
		}

		/* mouse_active screens forward background touches to virtual pointer */
		if (s_pages[i].ops->mouse_active) {
			lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
			lv_obj_add_event_cb(screen, tile_touch_cb, LV_EVENT_PRESSING,   NULL);
			lv_obj_add_event_cb(screen, tile_touch_cb, LV_EVENT_RELEASED,   NULL);
			lv_obj_add_event_cb(screen, tile_touch_cb, LV_EVENT_PRESS_LOST, NULL);
		}
	}

	/* Fire on_enter for the initial page */
	if (s_pages[0].ops->on_enter) {
		s_pages[0].ops->on_enter();
	}

	/* Return the first screen — ZMK calls lv_scr_load() on this */
	return s_pages[0].screen;
}
