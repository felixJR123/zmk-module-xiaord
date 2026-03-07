/* SPDX-License-Identifier: MIT */
#pragma once

#include <lvgl.h>
#include <stdbool.h>

/*
 * Create the 12-slot circle button ring on the given parent object.
 * Populates only the slots defined in the xiaord,home-buttons DT node
 * (or all 12 slots from the built-in fallback when no DT node exists).
 * Call once from page_home_create().
 */
void home_buttons_create(lv_obj_t *parent);

/*
 * Show or hide the button ring.
 * Manages the autohide timer and repeat timer accordingly.
 * Call from page_home_enter() (false) and tap overlay callback (true).
 */
void home_buttons_set_visible(bool visible);

/*
 * Pause all button-related timers (repeat + autohide).
 * Call from page_home_leave().
 */
void home_buttons_pause(void);
