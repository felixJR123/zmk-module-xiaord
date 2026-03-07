/*
 * SPDX-License-Identifier: MIT
 *
 * Clock screen: two sequential edit containers on the 240x240 circular display.
 *
 *   s_cont_date  (YYYY-MM-DD, shown on enter)
 *     "Set Date" title
 *     [YYYY roller] - [MM roller] - [DD roller]
 *     [✗ Cancel → HOME]   [✓ OK → s_cont_time]
 *
 *   s_cont_time  (HH:MM, initially hidden)
 *     "Set Time" title
 *     [HH roller] : [MM roller]
 *     [✗ Cancel → s_cont_date]   [✓ OK → rtc_set + HOME]
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <lvgl.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "page_ops.h"
#include "ui_btn.h"

/* ── RTC device ────────────────────────────────────────────────────────── */

static const struct device *s_rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

/* ── Widget handles ────────────────────────────────────────────────────── */

static lv_obj_t *s_cont_date, *s_cont_time;
static lv_obj_t *s_roller_hour, *s_roller_min;
static lv_obj_t *s_roller_year, *s_roller_mon, *s_roller_day;

/* ── Roller option strings ─────────────────────────────────────────────── */

static char s_opts_hour[72];    /* "00\n01\n...\n23" */
static char s_opts_min[180];    /* "00\n01\n...\n59" */
static char s_opts_year[60];    /* "2024\n...\n2035" */
static char s_opts_mon[36];     /* "01\n02\n...\n12" */
static char s_opts_day[93];     /* "01\n02\n...\n31" */

static void build_roller_opts(char *buf, int size, int from, int to, const char *fmt)
{
	int pos = 0;

	for (int i = from; i <= to; i++) {
		pos += snprintf(buf + pos, size - pos, fmt, i);
		if (i < to) {
			buf[pos++] = '\n';
		}
	}
}

/* ── Sakamoto's day-of-week (0=Sun..6=Sat) ────────────────────────────── */

static int day_of_week(int y, int m, int d)
{
	static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

	if (m < 3) {
		y--;
	}
	return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

/* ── Button callbacks ──────────────────────────────────────────────────── */

static void cb_date_cancel(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		ss_navigate_to(PAGE_HOME);
	}
}

static void cb_date_ok(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		lv_obj_add_flag(s_cont_date, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(s_cont_time, LV_OBJ_FLAG_HIDDEN);
	}
}

static void cb_time_cancel(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		lv_obj_add_flag(s_cont_time, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(s_cont_date, LV_OBJ_FLAG_HIDDEN);
	}
}

static void cb_time_ok(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}

	uint16_t hour = lv_roller_get_selected(s_roller_hour);
	uint16_t min  = lv_roller_get_selected(s_roller_min);
	uint16_t year = lv_roller_get_selected(s_roller_year) + 2024;
	uint16_t mon  = lv_roller_get_selected(s_roller_mon) + 1;
	uint16_t day  = lv_roller_get_selected(s_roller_day) + 1;

	struct rtc_time rt = {
		.tm_hour = hour,
		.tm_min  = min,
		.tm_sec  = 0,
		.tm_year = year - 1900,
		.tm_mon  = mon - 1,
		.tm_mday = day,
		.tm_wday = day_of_week(year, mon, day),
	};

	int err = rtc_set_time(s_rtc, &rt);

	if (err < 0) {
		LOG_ERR("rtc_set_time failed: %d", err);
	}

	ss_navigate_to(PAGE_HOME);
}

/* ── Transparent full-screen container ────────────────────────────────── */

static lv_obj_t *make_cont(lv_obj_t *parent)
{
	lv_obj_t *cont = lv_obj_create(parent);

	lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(cont, 0, 0);
	lv_obj_set_style_pad_all(cont, 0, 0);
	lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

	return cont;
}

/* ── Page create ───────────────────────────────────────────────────────── */

static int page_clock_create(lv_obj_t *screen)
{
	/* build roller option strings once at startup */
	build_roller_opts(s_opts_hour, sizeof(s_opts_hour), 0,    23,   "%02d");
	build_roller_opts(s_opts_min,  sizeof(s_opts_min),  0,    59,   "%02d");
	build_roller_opts(s_opts_year, sizeof(s_opts_year), 2024, 2035, "%04d");
	build_roller_opts(s_opts_mon,  sizeof(s_opts_mon),  1,    12,   "%02d");
	build_roller_opts(s_opts_day,  sizeof(s_opts_day),  1,    31,   "%02d");

	/* ── Date container (YYYY-MM-DD) ───────────────────────────────── */
	s_cont_date = make_cont(screen);

	lv_obj_t *lbl_date_title = lv_label_create(s_cont_date);
	lv_label_set_text(lbl_date_title, "Set Date");
	lv_obj_align(lbl_date_title, LV_ALIGN_TOP_MID, 0, 34);

	/* Year roller */
	s_roller_year = lv_roller_create(s_cont_date);
	lv_roller_set_options(s_roller_year, s_opts_year, LV_ROLLER_MODE_NORMAL);
	lv_roller_set_visible_row_count(s_roller_year, 3);
	lv_obj_set_width(s_roller_year, 62);
	lv_obj_align(s_roller_year, LV_ALIGN_TOP_MID, -53, 60);
	lv_obj_set_style_bg_opa(s_roller_year, LV_OPA_50, 0);

	lv_obj_t *dash1 = lv_label_create(s_cont_date);
	lv_label_set_text(dash1, "-");
	lv_obj_align(dash1, LV_ALIGN_TOP_MID, -19, 78);

	/* Month roller */
	s_roller_mon = lv_roller_create(s_cont_date);
	lv_roller_set_options(s_roller_mon, s_opts_mon, LV_ROLLER_MODE_INFINITE);
	lv_roller_set_visible_row_count(s_roller_mon, 3);
	lv_obj_set_width(s_roller_mon, 48);
	lv_obj_align(s_roller_mon, LV_ALIGN_TOP_MID, 0, 60);
	lv_obj_set_style_bg_opa(s_roller_mon, LV_OPA_50, 0);

	lv_obj_t *dash2 = lv_label_create(s_cont_date);
	lv_label_set_text(dash2, "-");
	lv_obj_align(dash2, LV_ALIGN_TOP_MID, 27, 78);

	/* Day roller */
	s_roller_day = lv_roller_create(s_cont_date);
	lv_roller_set_options(s_roller_day, s_opts_day, LV_ROLLER_MODE_NORMAL);
	lv_roller_set_visible_row_count(s_roller_day, 3);
	lv_obj_set_width(s_roller_day, 48);
	lv_obj_align(s_roller_day, LV_ALIGN_TOP_MID, 53, 60);
	lv_obj_set_style_bg_opa(s_roller_day, LV_OPA_50, 0);

	ui_create_btn(s_cont_date, LV_SYMBOL_HOME,
		      LV_ALIGN_BOTTOM_LEFT, 36, -30, 76, 32, LV_RADIUS_CIRCLE,
		      cb_date_cancel, NULL);

	ui_create_btn(s_cont_date, LV_SYMBOL_OK,
		      LV_ALIGN_BOTTOM_RIGHT, -36, -30, 76, 32, LV_RADIUS_CIRCLE,
		      cb_date_ok, NULL);

	/* ── Time container (HH:MM, initially hidden) ──────────────────── */
	s_cont_time = make_cont(screen);
	lv_obj_add_flag(s_cont_time, LV_OBJ_FLAG_HIDDEN);

	lv_obj_t *lbl_time_title = lv_label_create(s_cont_time);
	lv_label_set_text(lbl_time_title, "Set Time");
	lv_obj_align(lbl_time_title, LV_ALIGN_TOP_MID, 0, 34);

	/* Hour roller */
	s_roller_hour = lv_roller_create(s_cont_time);
	lv_roller_set_options(s_roller_hour, s_opts_hour, LV_ROLLER_MODE_INFINITE);
	lv_roller_set_visible_row_count(s_roller_hour, 3);
	lv_obj_set_width(s_roller_hour, 60);
	lv_obj_align(s_roller_hour, LV_ALIGN_TOP_MID, -38, 65);
	lv_obj_set_style_bg_opa(s_roller_hour, LV_OPA_50, 0);

	lv_obj_t *colon = lv_label_create(s_cont_time);
	lv_label_set_text(colon, ":");
	lv_obj_align(colon, LV_ALIGN_TOP_MID, 0, 80);

	/* Minute roller */
	s_roller_min = lv_roller_create(s_cont_time);
	lv_roller_set_options(s_roller_min, s_opts_min, LV_ROLLER_MODE_INFINITE);
	lv_roller_set_visible_row_count(s_roller_min, 3);
	lv_obj_set_width(s_roller_min, 60);
	lv_obj_align(s_roller_min, LV_ALIGN_TOP_MID, 38, 65);
	lv_obj_set_style_bg_opa(s_roller_min, LV_OPA_50, 0);

	ui_create_btn(s_cont_time, LV_SYMBOL_NEW_LINE,
		      LV_ALIGN_BOTTOM_LEFT, 36, -30, 76, 32, LV_RADIUS_CIRCLE,
		      cb_time_cancel, NULL);

	ui_create_btn(s_cont_time, LV_SYMBOL_OK,
		      LV_ALIGN_BOTTOM_RIGHT, -36, -30, 76, 32, LV_RADIUS_CIRCLE,
		      cb_time_ok, NULL);

	return 0;
}

/* ── Page lifecycle ────────────────────────────────────────────────────── */

static void page_clock_enter(void)
{
	/* populate rollers from current RTC time; fall back to 2026-01-01 00:00 */
	struct rtc_time rt;
	int hour = 0, min = 0, year = 2026, mon = 1, day = 1;

	if (rtc_get_time(s_rtc, &rt) == 0) {
		hour = rt.tm_hour;
		min  = rt.tm_min;
		year = rt.tm_year + 1900;
		mon  = rt.tm_mon + 1;
		day  = rt.tm_mday;
	}

	if (year < 2024) {
		year = 2024;
	}
	if (year > 2035) {
		year = 2035;
	}

	lv_roller_set_selected(s_roller_year, year - 2024, LV_ANIM_OFF);
	lv_roller_set_selected(s_roller_mon,  mon - 1,     LV_ANIM_OFF);
	lv_roller_set_selected(s_roller_day,  day - 1,     LV_ANIM_OFF);
	lv_roller_set_selected(s_roller_hour, hour,         LV_ANIM_OFF);
	lv_roller_set_selected(s_roller_min,  min,          LV_ANIM_OFF);

	lv_obj_clear_flag(s_cont_date, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(s_cont_time, LV_OBJ_FLAG_HIDDEN);
}

static void page_clock_leave(void)
{
	lv_obj_add_flag(s_cont_date, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(s_cont_time, LV_OBJ_FLAG_HIDDEN);
}

/* ── Page ops ──────────────────────────────────────────────────────────── */

const struct page_ops page_clock_ops = {
	.name         = "clock",
	.create       = page_clock_create,
	.on_enter     = page_clock_enter,
	.on_leave     = page_clock_leave,
};
