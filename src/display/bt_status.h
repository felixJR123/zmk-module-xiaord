/* SPDX-License-Identifier: MIT */

#pragma once

#include <lvgl.h>

/*
 * Initialize BT screen output status listener.
 *
 * output_lbl : label updated with endpoint/BLE connection state text.
 *              May be NULL (listener is a no-op until a label is registered).
 *
 * Must be called from the display work queue (inside page_bt_create).
 */
lv_obj_t *create_output_status_label(lv_obj_t *parent, const lv_font_t *font);
void bt_status_init(lv_obj_t *output_lbl);
