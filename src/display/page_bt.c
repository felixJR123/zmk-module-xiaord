/*
 * SPDX-License-Identifier: MIT
 *
 * Bluetooth settings screen.
 *
 * Layout (screen 240×240, center 120,120):
 *
 *   Profile radio buttons on circle perimeter (r=UI_CIRCLE_LAYOUT_RADIUS, up to 5):
 *     slots 10,11,0,1,2 → 10,11,12,1,2 o'clock
 *
 *   Inner area (r < ~80 from center):
 *     Upper   : output status label (font 36, y=-40)
 *     Row 1   : [OK] [-65,15]  [CLOSE] [0,15]  [TRASH] [+65,15]  action buttons
 *     Row 2   : [⌂] [-33,58]                   [USB]   [+33,58]  default buttons
 */

#include <lvgl.h>
#include <zmk/endpoints.h>
#include "page_ops.h"
#include "bt_status.h"
#include "ui_btn.h"

#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/ble.h>
#endif

/* ── Profile count ──────────────────────────────────────────────────────── */

#if IS_ENABLED(CONFIG_ZMK_BLE)
#define BT_PROFILE_COUNT_RAW \
	(CONFIG_BT_MAX_CONN - CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS)
#define BT_PROFILE_COUNT MIN(BT_PROFILE_COUNT_RAW, 5)
#else
#define BT_PROFILE_COUNT 0
#endif

/* ── State ──────────────────────────────────────────────────────────────── */

static int s_selected_profile;
static lv_obj_t *s_profile_btns[5];

/* ── Profile selection helper ───────────────────────────────────────────── */

static void set_selected_profile(int idx)
{
	s_selected_profile = idx;
	for (int i = 0; i < BT_PROFILE_COUNT; i++) {
		if (i == idx) {
			lv_obj_add_state(s_profile_btns[i], LV_STATE_CHECKED);
		} else {
			lv_obj_clear_state(s_profile_btns[i], LV_STATE_CHECKED);
		}
	}
}

/* ── Callbacks ──────────────────────────────────────────────────────────── */

static void profile_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	int idx = (int)(uintptr_t)lv_event_get_user_data(e);
	set_selected_profile(idx);
}

static void sel_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	ss_fire_behavior(INPUT_VIRTUAL_ZMK_BT_SEL_0 + s_selected_profile);
}

static void clr_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	ss_fire_behavior(INPUT_VIRTUAL_ZMK_BT_CLR_0 + s_selected_profile);
}

static void home_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	ss_navigate_to(PAGE_HOME);
}

static void disc_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	ss_fire_behavior(INPUT_VIRTUAL_ZMK_BT_DISC_0 + s_selected_profile);
}

static void usb_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	ss_fire_behavior(INPUT_VIRTUAL_SYM_USB);
}

/* ── Page create ────────────────────────────────────────────────────────── */

static int page_bt_create(lv_obj_t *screen)
{
	/* Profile radio buttons — upper half slots: 10,11,0,1,2 (clockwise) */
	static const int8_t s_upper_slots[5] = {10, 11, 0, 1, 2};
	int16_t pos[12][2];
	ui_circle_12_positions(pos, UI_CIRCLE_LAYOUT_RADIUS);
	for (int i = 0; i < BT_PROFILE_COUNT; i++) {
		char label[4];
		snprintf(label, sizeof(label), "%d", i);
		int slot = s_upper_slots[i];
		lv_obj_t *btn = ui_create_circle_btn(screen, label,
			pos[slot][0], pos[slot][1],
			profile_btn_cb, (void *)(uintptr_t)i);
		if (i == 0) {
			lv_obj_add_state(btn, LV_STATE_CHECKED);
		}
		s_profile_btns[i] = btn;
	}

	/* Output status label — upper inner area, large font */
	lv_obj_t *output_lbl = create_output_status_label(screen, &lv_font_montserrat_36);
	lv_obj_align(output_lbl, LV_ALIGN_CENTER, 0, -40);
	bt_status_init(output_lbl);

	/* Row 1: OK / DISC / CLR circle buttons */
	ui_create_circle_btn(screen, LV_SYMBOL_OK,    -65, 15, sel_btn_cb,  NULL);
	ui_create_circle_btn(screen, LV_SYMBOL_CLOSE,   0, 15, disc_btn_cb, NULL);
	ui_create_circle_btn(screen, LV_SYMBOL_TRASH,  65, 15, clr_btn_cb,  NULL);

	/* Row 2: HOME / USB circle buttons */
	ui_create_circle_btn(screen, LV_SYMBOL_HOME, -33, 58, home_btn_cb, NULL);
	ui_create_circle_btn(screen, LV_SYMBOL_USB,   33, 58, usb_btn_cb,  NULL);

	return 0;
}

/* ── Page enter ─────────────────────────────────────────────────────────── */

static void page_bt_enter(void)
{
	/* Sync radio button selection to current active BLE profile */
#if IS_ENABLED(CONFIG_ZMK_BLE)
	struct zmk_endpoint_instance ep = zmk_endpoint_get_selected();
	if (ep.transport == ZMK_TRANSPORT_BLE) {
		int idx = ep.ble.profile_index;
		if (idx >= 0 && idx < BT_PROFILE_COUNT) {
			set_selected_profile(idx);
		}
	}
#endif
}

/* ── Page ops ───────────────────────────────────────────────────────────── */

const struct page_ops page_bt_ops = {
	.name     = "bt",
	.create   = page_bt_create,
	.on_enter = page_bt_enter,
	.on_leave = NULL,
};
