/*
 * SPDX-License-Identifier: MIT
 *
 * Page interface for the xiaord multi-page system.
 * Each page implements this interface; the coordinator (status_screen.c)
 * owns the screens and manages page lifecycle.
 */

#pragma once

#include <lvgl.h>
#include <stdbool.h>
#include <stdint.h>

/* ── Page indices ──────────────────────────────────────────────────────── */

#define PAGE_HOME  0
#define PAGE_CLOCK 1
#define PAGE_BT    2

/* ── Key codes (used directly as INPUT_EV_ZMK_BEHAVIORS event codes) ────── */

/* INPUT_VIRTUAL_KEY_*, INPUT_VIRTUAL_ZMK_*, INPUT_VIRTUAL_SYM_*, INPUT_EV_ZMK_BEHAVIORS: */
#include "xiaord_input_codes.h"

/* Unified type for all virtual input codes (KEY, ZMK, SYM categories). */
typedef uint16_t input_virtual_code;

/* ── Per-page operations interface ────────────────────────────────────── */

struct page_ops {
	const char *name;
	int  (*create)(lv_obj_t *screen); /* create widgets on screen at init time */
	void (*on_enter)(void);           /* called when screen becomes active (nullable) */
	void (*on_leave)(void);           /* called when screen is navigated away from (nullable) */
};

/* ── Central coordinator API (implemented in status_screen.c) ─────────── */

/**
 * Programmatically navigate to a page by index.
 * @param page_idx  index in the page table (PAGE_HOME, PAGE_BT, ...)
 */
void ss_navigate_to(uint8_t page_idx);

/**
 * Report a key event through the virtual pointer input device.
 * Call with pressed=true on button press, pressed=false on release.
 * @param key     INPUT_VIRTUAL_KEY_0 .. INPUT_VIRTUAL_KEY_4
 * @param pressed true for key-down, false for key-up
 */
void ss_send_key(input_virtual_code key, bool pressed);

/**
 * Fire a ZMK behavior by sending a press+release pair.
 * Use for one-shot actions like sys_reset, bootloader, bt BT_CLR, etc.
 * @param code  INPUT_VIRTUAL_ZMK_* or INPUT_VIRTUAL_SYM_* constant from input_codes.h
 */
void ss_fire_behavior(input_virtual_code code);
