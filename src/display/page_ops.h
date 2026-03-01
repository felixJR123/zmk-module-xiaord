/*
 * SPDX-License-Identifier: MIT
 *
 * Page interface for the xiaord multi-page system.
 * Each page implements this interface; the coordinator (status_screen.c)
 * owns the screens and routes touch events based on mouse_active.
 */

#pragma once

#include <lvgl.h>
#include <stdbool.h>
#include <stdint.h>

/* ── Page indices ──────────────────────────────────────────────────────── */

#define PAGE_HOME     0
#define PAGE_CLOCK    1
#define PAGE_MACROPAD 2

/* ── Key codes (mapped to INPUT_KEY_x by coordinator) ─────────────────── */

enum ss_key_code {
	SS_KEY_1 = 1,
	SS_KEY_2 = 2,
	SS_KEY_COUNT
};

/* ── Per-page operations interface ────────────────────────────────────── */

struct page_ops {
	const char *name;
	bool mouse_active;                /* true → screen touch forwarded to virtual pointer */
	int  (*create)(lv_obj_t *screen); /* create widgets on screen at init time */
	void (*on_enter)(void);           /* called when screen becomes active (nullable) */
	void (*on_leave)(void);           /* called when screen is navigated away from (nullable) */
};

/* ── Central coordinator API (implemented in status_screen.c) ─────────── */

/**
 * Programmatically navigate to a page by index.
 * @param page_idx  index in the page table (PAGE_HOME, PAGE_MACROPAD, ...)
 */
void ss_navigate_to(uint8_t page_idx);

/**
 * Report a key event through the virtual pointer input device.
 * Call with pressed=true on button press, pressed=false on release.
 * @param key     key code from ss_key_code enum
 * @param pressed true for key-down, false for key-up
 */
void ss_send_key(enum ss_key_code key, bool pressed);
