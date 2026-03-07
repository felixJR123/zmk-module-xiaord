/* SPDX-License-Identifier: MIT */

#pragma once

#include <lvgl.h>

/*
 * Initialize home-screen status listeners.
 *
 * output_lbl      : label for endpoint/connection status (may be NULL)
 * periph_bat_arcs : array of lv_arc widgets, one per peripheral
 * periph_bat_lbls : array of labels inside each arc, one per peripheral
 *                   (lengths = ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT)
 *
 * Must be called from the display work queue (inside page_home_create).
 */
void home_status_init(lv_obj_t *output_lbl,
		      lv_obj_t **periph_bat_arcs,
		      lv_obj_t **periph_bat_lbls);
