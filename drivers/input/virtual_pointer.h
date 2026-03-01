/*
 * SPDX-License-Identifier: MIT
 *
 * Public API for the zmk,virtual-pointer driver.
 *
 * The display layer (status_screen.c) calls zmk_virtual_pointer_feed() on each
 * LVGL indev event to forward touch coordinates into the pointer pipeline.
 */

#pragma once

#include <zephyr/device.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Feed a touch frame into the virtual pointer.
 *
 * Must be called from the LVGL display thread (or with appropriate locking).
 *
 * @param dev      virtual-pointer device handle (DT_NODELABEL(vpointer))
 * @param x        absolute X coordinate in display pixels
 * @param y        absolute Y coordinate in display pixels
 * @param pressed  true while finger is on screen, false on release
 */
void zmk_virtual_pointer_feed(const struct device *dev, int16_t x, int16_t y, bool pressed);
