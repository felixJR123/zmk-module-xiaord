/* SPDX-License-Identifier: MIT */
/*
 * Unified endpoint/BLE status listener.
 *
 * Subscribes to zmk_endpoint_changed and zmk_ble_active_profile_changed
 * once, then fans out to all registered page callbacks.
 */

#pragma once

#include <lvgl.h>
#include <zmk/endpoints.h>

/* ── State struct passed to every registered callback ─────────────────── */

struct endpoint_state {
	struct zmk_endpoint_instance selected_endpoint;
	struct zmk_endpoint_instance preferred_endpoint;
	bool active_profile_connected;
	bool active_profile_bonded;
	bool profiles_connected[5];
	bool profiles_bonded[5];
	int active_ble_profile;
};

typedef void (*endpoint_status_cb_t)(struct endpoint_state state);

/**
 * Register a callback to be notified on every endpoint state change.
 * The first registration also starts the ZMK event listener.
 * Must be called from the display work queue (inside a page_*_create()).
 */
void endpoint_status_register_cb(endpoint_status_cb_t cb);

/**
 * Shared text-formatting helper: write endpoint state into lbl.
 * Factored out so both home and BT pages render identical status text.
 */
void endpoint_status_update_label(lv_obj_t *lbl, struct endpoint_state state);

/**
 * Factory: create a label widget suitable for output status display.
 * Caller is responsible for positioning the returned object.
 */
lv_obj_t *create_output_status_label(lv_obj_t *parent, const lv_font_t *font);

/**
 * Force an immediate re-read of endpoint state and fan out to all registered
 * callbacks.  Safe to call from the LVGL/display work queue (e.g. from a
 * one-shot lv_timer callback after firing a ZMK behavior).
 */
void endpoint_status_request_refresh(void);
