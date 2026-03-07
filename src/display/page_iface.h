/* SPDX-License-Identifier: MIT */
/*
 * Per-page interface for the xiaord multi-page system.
 * Each page implements struct page_ops; the coordinator (status_screen.c)
 * owns the screens and manages page lifecycle.
 */

#pragma once

#include <lvgl.h>

/* ── Page indices ──────────────────────────────────────────────────────── */

#define PAGE_HOME  0
#define PAGE_CLOCK 1
#define PAGE_BT    2

/* ── Per-page operations interface ────────────────────────────────────── */

struct page_ops {
	const char *name;
	int  (*create)(lv_obj_t *screen); /* create widgets on screen at init time */
	void (*on_enter)(void);           /* called when screen becomes active (nullable) */
	void (*on_leave)(void);           /* called when screen is navigated away from (nullable) */
};
