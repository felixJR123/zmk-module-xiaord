/*
 * SPDX-License-Identifier: MIT
 *
 * Home screen status listeners — output endpoint and peripheral batteries.
 *
 * Uses ZMK_DISPLAY_WIDGET_LISTENER with static label pointers instead of
 * a sys_slist_t widget list, since there is exactly one home page.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/central.h>

#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/ble.h>
#include <zmk/events/ble_active_profile_changed.h>
#endif

BUILD_ASSERT(IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING),
	     "home_status requires CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING=y");

/* ── Static LVGL object references ─────────────────────────────────────── */

static lv_obj_t *s_output_lbl;
static lv_obj_t *s_periph_bat_lbl[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];

/* ── Output status listener ─────────────────────────────────────────────── */

struct output_status_state {
	struct zmk_endpoint_instance selected_endpoint;
	enum zmk_transport preferred_transport;
	bool active_profile_connected;
	bool active_profile_bonded;
};

static struct output_status_state output_get_state(const zmk_event_t *_eh)
{
	return (struct output_status_state){
		.selected_endpoint = zmk_endpoint_get_selected(),
		.preferred_transport = zmk_endpoint_get_preferred_transport(),
#if IS_ENABLED(CONFIG_ZMK_BLE)
		.active_profile_connected = zmk_ble_active_profile_is_connected(),
		.active_profile_bonded = !zmk_ble_active_profile_is_open(),
#endif
	};
}

static void output_update_cb(struct output_status_state state)
{
	if (!s_output_lbl) {
		return;
	}

	char text[20] = {};

	enum zmk_transport transport = state.selected_endpoint.transport;
	bool connected = transport != ZMK_TRANSPORT_NONE;

	if (!connected) {
		transport = state.preferred_transport;
	}

	switch (transport) {
	case ZMK_TRANSPORT_NONE:
		strcat(text, LV_SYMBOL_CLOSE);
		break;

	case ZMK_TRANSPORT_USB:
		strcat(text, LV_SYMBOL_USB);
		if (!connected) {
			strcat(text, " " LV_SYMBOL_CLOSE);
		}
		break;

	case ZMK_TRANSPORT_BLE:
		if (state.active_profile_bonded) {
			if (state.active_profile_connected) {
				snprintf(text, sizeof(text), LV_SYMBOL_WIFI " %i " LV_SYMBOL_OK,
					 state.selected_endpoint.ble.profile_index + 1);
			} else {
				snprintf(text, sizeof(text), LV_SYMBOL_WIFI " %i " LV_SYMBOL_CLOSE,
					 state.selected_endpoint.ble.profile_index + 1);
			}
		} else {
			snprintf(text, sizeof(text), LV_SYMBOL_WIFI " %i " LV_SYMBOL_SETTINGS,
				 state.selected_endpoint.ble.profile_index + 1);
		}
		break;
	}

	lv_label_set_text(s_output_lbl, text);
}

ZMK_DISPLAY_WIDGET_LISTENER(home_output_status, struct output_status_state, output_update_cb,
			    output_get_state)
ZMK_SUBSCRIPTION(home_output_status, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(home_output_status, zmk_ble_active_profile_changed);
#endif

/* ── Peripheral battery listener ─────────────────────────────────────────── */

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
		lv_obj_t *lbl = s_periph_bat_lbl[i];

		if (!lbl) {
			continue;
		}

		if (state.valid[i]) {
			lv_label_set_text_fmt(lbl, "P%d: %d%%", i, state.level[i]);
		} else {
			lv_label_set_text_fmt(lbl, "P%d: --", i);
		}
	}
}

ZMK_DISPLAY_WIDGET_LISTENER(home_periph_battery, struct periph_bat_state, periph_bat_update_cb,
			    periph_bat_get_state)
ZMK_SUBSCRIPTION(home_periph_battery, zmk_peripheral_battery_state_changed);

/* ── Public init ─────────────────────────────────────────────────────────── */

void home_status_init(lv_obj_t *output_lbl, lv_obj_t **periph_bat_lbls)
{
	s_output_lbl = output_lbl;

	for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; i++) {
		s_periph_bat_lbl[i] = periph_bat_lbls[i];
	}

	home_output_status_init();
	home_periph_battery_init();
}
