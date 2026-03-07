/* SPDX-License-Identifier: MIT */
/*
 * Peripheral battery status listener for the home screen.
 *
 * Subscribes to zmk_peripheral_battery_state_changed and updates
 * the arc gauge widgets passed at init time.
 */

#pragma once

#include <lvgl.h>

/**
 * Initialize peripheral battery listener.
 *
 * arcs  : array of lv_arc widgets, one per peripheral
 * lbls  : array of labels inside each arc, one per peripheral
 *         (array length = ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT)
 *
 * Must be called from the display work queue (inside page_home_create).
 */
void battery_status_init(lv_obj_t **arcs, lv_obj_t **lbls);
