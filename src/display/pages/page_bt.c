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
 *     Row 1   : [USB] [-33,15]  [TRASH] [+33,15]
 *     Row 2   : [⌂] [0,58]
 */

#include <lvgl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include "page_iface.h"
#include "display_api.h"
#include "endpoint_status.h"
#include "ui_btn.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

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

static lv_obj_t *s_profile_btns[5];
static lv_obj_t *s_output_lbl;

/* ── Endpoint status callback ────────────────────────────────────────────── */

static void bt_endpoint_cb(struct endpoint_state state)
{
	endpoint_status_update_label(s_output_lbl, state);

	int active = state.active_ble_profile;
	bool prefer_bt = (state.preferred_endpoint.transport == ZMK_TRANSPORT_BLE);

	LOG_DBG("preferred=%d selected=%d active_profile=%d prefer_bt=%d",
		state.preferred_endpoint.transport,
		state.selected_endpoint.transport, active, prefer_bt);

	for (int i = 0; i < BT_PROFILE_COUNT; i++) {
		lv_obj_clear_state(s_profile_btns[i],
				   LV_STATE_CHECKED | LV_STATE_USER_1 |
				   LV_STATE_USER_2 | LV_STATE_USER_3);

		bool selected  = (i == active);
		bool bonded    = state.profiles_bonded[i];
		bool connected = state.profiles_connected[i];

		if (prefer_bt) {
			if (selected) {
				if (bonded && connected) {
					lv_obj_add_state(s_profile_btns[i], LV_STATE_CHECKED); /* blue */
				} else if (bonded) {
					lv_obj_add_state(s_profile_btns[i], LV_STATE_USER_3); /* red */
				} else {
					lv_obj_add_state(s_profile_btns[i], LV_STATE_USER_1); /* yellow */
				}
			} else {
				if (bonded) {
					lv_obj_add_state(s_profile_btns[i], LV_STATE_USER_2); /* green */
				}
				/* !bonded → white (default) */
			}
		} else {
			/* preferred is USB: bonded always shows green regardless of selected */
			if (bonded) {
				lv_obj_add_state(s_profile_btns[i], LV_STATE_USER_2); /* green */
			} else if (selected) {
				lv_obj_add_state(s_profile_btns[i], LV_STATE_USER_1); /* yellow */
			}
			/* !bonded && !selected → white (default) */
		}

		LOG_DBG("profile[%d]: prefer_bt=%d selected=%d bonded=%d connected=%d -> %s",
			i, prefer_bt, selected, bonded, connected,
			prefer_bt
				? (selected
					? (bonded && connected ? "blue" : bonded ? "red" : "yellow")
					: (bonded ? "green" : "white"))
				: (bonded ? "green" : selected ? "yellow" : "white"));
	}
}

/* ── Callbacks ──────────────────────────────────────────────────────────── */

static void refresh_timer_cb(lv_timer_t *timer)
{
	endpoint_status_request_refresh();
	lv_timer_delete(timer);
}

static void profile_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	int idx = (int)(uintptr_t)lv_event_get_user_data(e);

	ss_fire_behavior(INPUT_VIRTUAL_ZMK_OUT_BLE);
	ss_fire_behavior(INPUT_VIRTUAL_ZMK_BT_SEL_0 + idx);
	lv_timer_create(refresh_timer_cb, 100, NULL);
}

static void clr_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	ss_fire_behavior(INPUT_VIRTUAL_ZMK_BT_CLR);
	lv_timer_create(refresh_timer_cb, 100, NULL);
}

static void home_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	ss_navigate_to(PAGE_HOME);
}

static void usb_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	ss_fire_behavior(INPUT_VIRTUAL_ZMK_OUT_USB);
	lv_timer_create(refresh_timer_cb, 100, NULL);
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
		snprintf(label, sizeof(label), "%d", i + 1);
		int slot = s_upper_slots[i];
		lv_obj_t *btn = ui_create_circle_btn(screen, label,
			pos[slot][0], pos[slot][1],
			profile_btn_cb, (void *)(uintptr_t)i);
		s_profile_btns[i] = btn;
	}
	/* Output status label — upper inner area, large font */
	s_output_lbl = create_output_status_label(screen, &lv_font_montserrat_36);
	lv_obj_align(s_output_lbl, LV_ALIGN_CENTER, 0, -40);
	endpoint_status_register_cb(bt_endpoint_cb);

	/* Row 1: USB / CLR */
	ui_create_circle_btn(screen, LV_SYMBOL_USB,   -33, 15, usb_btn_cb,  NULL);
	ui_create_circle_btn(screen, LV_SYMBOL_TRASH,  33, 15, clr_btn_cb,  NULL);

	/* Row 2: HOME */
	ui_create_circle_btn(screen, LV_SYMBOL_HOME,    0, 58, home_btn_cb, NULL);

	return 0;
}

/* ── Page ops ───────────────────────────────────────────────────────────── */

const struct page_ops page_bt_ops = {
	.name     = "bt",
	.create   = page_bt_create,
	.on_enter = NULL,
	.on_leave = NULL,
};
