/*
 * SPDX-License-Identifier: MIT
 *
 * Public API for the zmk,virtual-key-source driver.
 *
 * The display layer (status_screen.c) uses input_report_key() directly via
 * <zephyr/input/input.h> to emit INPUT_KEY_x events on this device.
 *
 * zmk_virtual_pointer_feed() (cursor/gesture API) is preserved below in a
 * #if 0 guard for future reference.
 */

#pragma once

#include <zephyr/device.h>
#include <stdint.h>
#include <stdbool.h>

#if 0
/**
 * Feed a touch frame into the virtual pointer (cursor mode).
 * Currently unused — touch is handled entirely by LVGL.
 */
void zmk_virtual_pointer_feed(const struct device *dev, int16_t x, int16_t y, bool pressed);
#endif
