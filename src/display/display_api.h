/* SPDX-License-Identifier: MIT */
/*
 * Central coordinator API implemented in status_screen.c.
 * Also exposes the input_virtual_code type and xiaord input code constants.
 */

#pragma once

#include <stdint.h>

/* INPUT_VIRTUAL_ZMK_*, INPUT_VIRTUAL_POS_*, INPUT_EV_ZMK_BEHAVIORS: */
#include "xiaord_input_codes.h"

/* Unified type for all virtual input codes (POS, ZMK categories). */
typedef uint16_t input_virtual_code;

/**
 * Programmatically navigate to a page by index.
 * @param page_idx  index in the page table (PAGE_HOME, PAGE_BT, ...)
 */
void ss_navigate_to(uint8_t page_idx);

/**
 * Fire a ZMK behavior by sending a press+release pair.
 * @param code  INPUT_VIRTUAL_POS_* or INPUT_VIRTUAL_ZMK_* constant
 */
void ss_fire_behavior(input_virtual_code code);
