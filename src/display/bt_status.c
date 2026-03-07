/*
 * SPDX-License-Identifier: MIT
 *
 * BT settings screen output status listener.
 *
 * Mirrors the output_update_cb logic from home_status.c, but registered
 * under a distinct listener name so both screens can coexist.
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

/* ── Static LVGL object references ─────────────────────────────────────── */

static lv_obj_t *s_output_lbl;
static lv_obj_t **s_profile_btns;
static int s_profile_btn_count;

/* ── Output status listener ─────────────────────────────────────────────── */

struct bt_output_status_state {
	struct zmk_endpoint_instance selected_endpoint;
	enum zmk_transport preferred_transport;
	bool active_profile_connected;
	bool active_profile_bonded;
};

static struct bt_output_status_state bt_output_get_state(const zmk_event_t *_eh)
{
	return (struct bt_output_status_state){
		.selected_endpoint = zmk_endpoint_get_selected(),
		.preferred_transport = zmk_endpoint_get_preferred_transport(),
#if IS_ENABLED(CONFIG_ZMK_BLE)
		.active_profile_connected = zmk_ble_active_profile_is_connected(),
		.active_profile_bonded = !zmk_ble_active_profile_is_open(),
#endif
	};
}

static void bt_output_update_cb(struct bt_output_status_state state)
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
				snprintf(text, sizeof(text), LV_SYMBOL_BLUETOOTH " %i " LV_SYMBOL_OK,
					 state.selected_endpoint.ble.profile_index + 1);
			} else {
				snprintf(text, sizeof(text), LV_SYMBOL_BLUETOOTH " %i " LV_SYMBOL_CLOSE,
					 state.selected_endpoint.ble.profile_index + 1);
			}
		} else {
			snprintf(text, sizeof(text), LV_SYMBOL_BLUETOOTH " %i " LV_SYMBOL_SETTINGS,
				 state.selected_endpoint.ble.profile_index + 1);
		}
		break;
	}

	lv_label_set_text(s_output_lbl, text);

	/* Update profile button CHECKED states */
	if (s_profile_btns) {
		int active = -1;
		if (transport == ZMK_TRANSPORT_BLE) {
			active = state.selected_endpoint.ble.profile_index;
		}
		for (int i = 0; i < s_profile_btn_count; i++) {
			if (i == active) {
				lv_obj_add_state(s_profile_btns[i], LV_STATE_CHECKED);
				/* Connection established — clear pending on all buttons */
				for (int j = 0; j < s_profile_btn_count; j++) {
					lv_obj_clear_state(s_profile_btns[j], LV_STATE_USER_1);
				}
			} else {
				lv_obj_clear_state(s_profile_btns[i], LV_STATE_CHECKED);
			}
		}
	}
}

ZMK_DISPLAY_WIDGET_LISTENER(bt_output_status, struct bt_output_status_state, bt_output_update_cb,
			    bt_output_get_state)
ZMK_SUBSCRIPTION(bt_output_status, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(bt_output_status, zmk_ble_active_profile_changed);
#endif

/* ── Public helpers ──────────────────────────────────────────────────────── */

lv_obj_t *create_output_status_label(lv_obj_t *parent, const lv_font_t *font)
{
	lv_obj_t *lbl = lv_label_create(parent);
	lv_label_set_text(lbl, "");
	if (font) {
		lv_obj_set_style_text_font(lbl, font, 0);
	}
	return lbl;
}

void bt_status_init(lv_obj_t *output_lbl)
{
	s_output_lbl = output_lbl;
	bt_output_status_init();
}

void bt_status_set_profile_btns(lv_obj_t **btns, int count)
{
	s_profile_btns = btns;
	s_profile_btn_count = count;
}
