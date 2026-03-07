/*
 * SPDX-License-Identifier: MIT
 *
 * Peripheral battery status listener — home screen arc gauges.
 *
 * Extracted from home_status.c so battery and endpoint concerns are
 * in separate compilation units.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/central.h>

#include "battery_status.h"

BUILD_ASSERT(IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING),
	     "battery_status requires CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING=y");

/* ── Static LVGL object references ─────────────────────────────────────── */

static lv_obj_t *s_bat_arc[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
static lv_obj_t *s_bat_lbl[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];

/* ── ZMK listener ───────────────────────────────────────────────────────── */

struct periph_bat_state {
	uint8_t level[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
	bool valid[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
};

static struct periph_bat_state periph_bat_get_state(const zmk_event_t *eh)
{
	struct periph_bat_state state = {};

	for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; i++) {
		uint8_t level;
		int rc = zmk_split_central_get_peripheral_battery_level(i, &level);

		state.level[i] = level;
		state.valid[i] = (rc == 0);
	}

	return state;
}

static void periph_bat_update_cb(struct periph_bat_state state)
{
	for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; i++) {
		lv_obj_t *arc = s_bat_arc[i];
		lv_obj_t *lbl = s_bat_lbl[i];

		if (state.valid[i]) {
			if (arc) {
				lv_arc_set_value(arc, state.level[i]);
			}
			if (lbl) {
				lv_label_set_text_fmt(lbl, "%d%%", state.level[i]);
			}
		} else {
			if (arc) {
				lv_arc_set_value(arc, 0);
			}
			if (lbl) {
				lv_label_set_text(lbl, "--");
			}
		}
	}
}

ZMK_DISPLAY_WIDGET_LISTENER(periph_battery, struct periph_bat_state,
			    periph_bat_update_cb, periph_bat_get_state)
ZMK_SUBSCRIPTION(periph_battery, zmk_peripheral_battery_state_changed);

/* ── Public init ────────────────────────────────────────────────────────── */

void battery_status_init(lv_obj_t **arcs, lv_obj_t **lbls)
{
	for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; i++) {
		s_bat_arc[i] = arcs[i];
		s_bat_lbl[i] = lbls[i];
	}

	periph_battery_init();
}
