/*
 * SPDX-License-Identifier: MIT
 *
 * Unified endpoint/BLE status listener.
 *
 * Replaces the duplicate home_output_status and bt_output_status listeners.
 * A single ZMK_DISPLAY_WIDGET_LISTENER reads endpoint state on every
 * zmk_endpoint_changed / zmk_ble_active_profile_changed event and fans
 * out to all callbacks registered via endpoint_status_register_cb().
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/events/endpoint_changed.h>

#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/ble.h>
#include <zmk/events/ble_active_profile_changed.h>
#endif

#include "endpoint_status.h"

/* ── Callback registry ──────────────────────────────────────────────────── */

#define MAX_ENDPOINT_CBS 4

static endpoint_status_cb_t s_callbacks[MAX_ENDPOINT_CBS];
static int s_cb_count;

/* ── ZMK state extraction ───────────────────────────────────────────────── */

static struct endpoint_state endpoint_get_state(const zmk_event_t *_eh)
{
	struct endpoint_state state = {
		.selected_endpoint = zmk_endpoint_get_selected(),
		.preferred_endpoint = zmk_endpoint_get_preferred(),
#if IS_ENABLED(CONFIG_ZMK_BLE)
		.active_profile_connected = zmk_ble_active_profile_is_connected(),
		.active_profile_bonded = !zmk_ble_active_profile_is_open(),
		.active_ble_profile = zmk_ble_active_profile_index(),
#endif
	};
#if IS_ENABLED(CONFIG_ZMK_BLE)
	for (int i = 0; i < MIN(5, ZMK_BLE_PROFILE_COUNT); i++) {
		state.profiles_connected[i] = zmk_ble_profile_is_connected(i);
		state.profiles_bonded[i] = !zmk_ble_profile_is_open(i);
	}
#endif
	return state;
}

static void endpoint_update_cb(struct endpoint_state state)
{
	for (int i = 0; i < s_cb_count; i++) {
		s_callbacks[i](state);
	}
}

ZMK_DISPLAY_WIDGET_LISTENER(endpoint_status, struct endpoint_state,
			    endpoint_update_cb, endpoint_get_state)
ZMK_SUBSCRIPTION(endpoint_status, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(endpoint_status, zmk_ble_active_profile_changed);
#endif

/* ── Public API ─────────────────────────────────────────────────────────── */

void endpoint_status_register_cb(endpoint_status_cb_t cb)
{
	if (s_cb_count == 0) {
		/* Start the ZMK event subscription on first registration.
		 * All page create() calls happen synchronously before the
		 * display work item fires, so subsequent registrations are
		 * safe — they will all receive the initial state update. */
		endpoint_status_init();
	}
	if (s_cb_count < MAX_ENDPOINT_CBS) {
		s_callbacks[s_cb_count++] = cb;
	}
}

void endpoint_status_update_label(lv_obj_t *lbl, struct endpoint_state state)
{
	if (!lbl) {
		return;
	}

	char text[24] = {};

	enum zmk_transport transport = state.selected_endpoint.transport;
	bool connected = transport != ZMK_TRANSPORT_NONE;

	if (!connected) {
		transport = state.preferred_endpoint.transport;
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
				snprintf(text, sizeof(text),
					 LV_SYMBOL_BLUETOOTH " %i " LV_SYMBOL_OK,
					 state.selected_endpoint.ble.profile_index + 1);
			} else {
				snprintf(text, sizeof(text),
					 LV_SYMBOL_BLUETOOTH " %i " LV_SYMBOL_CLOSE,
					 state.selected_endpoint.ble.profile_index + 1);
			}
		} else {
			snprintf(text, sizeof(text),
				 LV_SYMBOL_BLUETOOTH " %i " LV_SYMBOL_SETTINGS,
				 state.selected_endpoint.ble.profile_index + 1);
		}
		break;
	}

	LOG_DBG("preferred=%d selected=%d", state.preferred_endpoint.transport,
		state.selected_endpoint.transport);

	if (!zmk_endpoint_instance_eq(state.preferred_endpoint, state.selected_endpoint)) {
		strcat(text, " " LV_SYMBOL_WARNING);
	}

	lv_label_set_text(lbl, text);
}

lv_obj_t *create_output_status_label(lv_obj_t *parent, const lv_font_t *font)
{
	lv_obj_t *lbl = lv_label_create(parent);
	lv_label_set_text(lbl, "");
	if (font) {
		lv_obj_set_style_text_font(lbl, font, 0);
	}
	return lbl;
}
