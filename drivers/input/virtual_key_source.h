/*
 * SPDX-License-Identifier: MIT
 *
 * Public API for the zmk,virtual-key-source driver.
 *
 * The display layer (status_screen.c) uses input_report_key() directly via
 * <zephyr/input/input.h> to emit INPUT_KEY_x events on this device.
 */

#pragma once

#include <zephyr/device.h>
#include <stdint.h>
#include <stdbool.h>
