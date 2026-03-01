/*
 * SPDX-License-Identifier: MIT
 *
 * Clock screen: reads time from the PCF8563 RTC (DT alias "rtc") and
 * displays it on the 240x240 circular display.
 *
 * Two containers share the screen; only one is visible at a time:
 *
 *   s_cont_display  (normal view)
 *     HH:MM  (Montserrat 48)
 *     YYYY-MM-DD  (default font)
 *     [◁ Back]   [✎ Set]   [▶]
 *
 *   s_cont_edit  (time-set mode, initially hidden)
 *     [HH roller] : [MM roller]
 *     [YYYY roller] - [MM roller] - [DD roller]
 *     [✗ Cancel]                       [✓ OK]
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <lvgl.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "page_ops.h"

/* ── RTC device ────────────────────────────────────────────────────────── */

static const struct device *s_rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

/* ── Widget handles ────────────────────────────────────────────────────── */

static lv_obj_t *s_lbl_time;
static lv_obj_t *s_lbl_date;
static lv_timer_t *s_timer;

/* edit mode */
static bool s_edit_mode;
static lv_obj_t *s_cont_display, *s_cont_edit;
static lv_obj_t *s_roller_hour, *s_roller_min;
static lv_obj_t *s_roller_year, *s_roller_mon, *s_roller_day;

/* ── Roller option strings ─────────────────────────────────────────────── */

static char s_opts_hour[72];   /* "00\n01\n...\n23" */
static char s_opts_min[180];   /* "00\n01\n...\n59" */
static char s_opts_year[60];   /* "2024\n...\n2035" */
static char s_opts_mon[36];    /* "01\n02\n...\n12" */
static char s_opts_day[93];    /* "01\n02\n...\n31" */

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

/* ── Timer callback ────────────────────────────────────────────────────── */

static void update_clock(lv_timer_t *t)
{
	ARG_UNUSED(t);

	struct rtc_time time;
	int err = rtc_get_time(s_rtc, &time);

	if (err == -ENODATA) {
		lv_label_set_text(s_lbl_time, "--:--");
		lv_label_set_text(s_lbl_date, "RTC not set");
		return;
	}
	if (err < 0) {
		LOG_ERR("rtc_get_time failed: %d", err);
		lv_label_set_text(s_lbl_time, "ERR");
		lv_label_set_text(s_lbl_date, "");
		return;
	}

	char time_buf[6];
	char date_buf[11];

	snprintf(time_buf, sizeof(time_buf), "%02d:%02d",
		 time.tm_hour, time.tm_min);
	snprintf(date_buf, sizeof(date_buf), "%04d-%02d-%02d",
		 time.tm_year + 1900, time.tm_mon + 1, time.tm_mday);

	lv_label_set_text(s_lbl_time, time_buf);
	lv_label_set_text(s_lbl_date, date_buf);
}

/* ── Mode transitions ──────────────────────────────────────────────────── */

static void enter_edit_mode(void)
{
	lv_timer_pause(s_timer);

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

	lv_roller_set_selected(s_roller_hour, hour,        LV_ANIM_OFF);
	lv_roller_set_selected(s_roller_min,  min,         LV_ANIM_OFF);
	lv_roller_set_selected(s_roller_year, year - 2024, LV_ANIM_OFF);
	lv_roller_set_selected(s_roller_mon,  mon - 1,     LV_ANIM_OFF);
	lv_roller_set_selected(s_roller_day,  day - 1,     LV_ANIM_OFF);

	lv_obj_add_flag(s_cont_display, LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(s_cont_edit, LV_OBJ_FLAG_HIDDEN);
	s_edit_mode = true;
}

static void leave_edit_mode(void)
{
	s_edit_mode = false;
	lv_obj_add_flag(s_cont_edit, LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(s_cont_display, LV_OBJ_FLAG_HIDDEN);
	update_clock(NULL);
	lv_timer_resume(s_timer);
}

/* ── Button callbacks ──────────────────────────────────────────────────── */

static void cb_back_btn(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		ss_navigate_to(PAGE_HOME);
	}
}

static void cb_next_btn(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		ss_navigate_to(PAGE_MACROPAD);
	}
}

static void cb_set_btn(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		enter_edit_mode();
	}
}

static void cb_cancel_btn(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		leave_edit_mode();
	}
}

static void cb_ok_btn(lv_event_t *e)
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

	leave_edit_mode();
}

/* ── Button factory ────────────────────────────────────────────────────── */

static void make_btn(lv_obj_t *parent, const char *text,
		     int w, int h,
		     lv_align_t align, int x_ofs, int y_ofs,
		     lv_event_cb_t cb)
{
	lv_obj_t *btn = lv_obj_create(parent);

	lv_obj_set_size(btn, w, h);
	lv_obj_align(btn, align, x_ofs, y_ofs);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, NULL);

	lv_obj_t *lbl = lv_label_create(btn);

	lv_label_set_text(lbl, text);
	lv_obj_center(lbl);
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

	/* ── Display container ─────────────────────────────────────────── */
	s_cont_display = make_cont(screen);

	/* Time label — large font, slightly above centre */
	s_lbl_time = lv_label_create(s_cont_display);
	lv_obj_set_style_text_font(s_lbl_time, &lv_font_montserrat_48, 0);
	lv_label_set_text(s_lbl_time, "--:--");
	lv_obj_align(s_lbl_time, LV_ALIGN_CENTER, 0, -24);

	/* Date label — default font, below time */
	s_lbl_date = lv_label_create(s_cont_display);
	lv_label_set_text(s_lbl_date, "----.--.--");
	lv_obj_align(s_lbl_date, LV_ALIGN_CENTER, 0, 36);

	/* Back / Set / Next buttons */
	make_btn(s_cont_display, LV_SYMBOL_LEFT " Back",
		 80, 36, LV_ALIGN_BOTTOM_LEFT,  24,  -16, cb_back_btn);
	make_btn(s_cont_display, LV_SYMBOL_EDIT,
		 48, 36, LV_ALIGN_BOTTOM_MID,   0,   -16, cb_set_btn);
	make_btn(s_cont_display, LV_SYMBOL_RIGHT,
		 48, 36, LV_ALIGN_BOTTOM_RIGHT, -24, -16, cb_next_btn);

	/* ── Edit container (hidden until Set is pressed) ──────────────── */
	s_cont_edit = make_cont(screen);
	lv_obj_add_flag(s_cont_edit, LV_OBJ_FLAG_HIDDEN);

	/*
	 * Time row  y≈20
	 *   [HH 56px]  :  [MM 56px]
	 * Date row  y≈100
	 *   [YYYY 62px] - [MM 48px] - [DD 48px]
	 * Button row  y≈190
	 *   [✗ Cancel 72px]          [✓ OK 72px]
	 */

	/* Hour roller */
	s_roller_hour = lv_roller_create(s_cont_edit);
	lv_roller_set_options(s_roller_hour, s_opts_hour, LV_ROLLER_MODE_INFINITE);
	lv_roller_set_visible_row_count(s_roller_hour, 3);
	lv_obj_set_width(s_roller_hour, 56);
	lv_obj_align(s_roller_hour, LV_ALIGN_TOP_MID, -34, 20);

	/* Colon separator */
	lv_obj_t *colon = lv_label_create(s_cont_edit);
	lv_label_set_text(colon, ":");
	lv_obj_set_style_text_font(colon, &lv_font_montserrat_48, 0);
	lv_obj_align(colon, LV_ALIGN_TOP_MID, 0, 30);

	/* Minute roller */
	s_roller_min = lv_roller_create(s_cont_edit);
	lv_roller_set_options(s_roller_min, s_opts_min, LV_ROLLER_MODE_INFINITE);
	lv_roller_set_visible_row_count(s_roller_min, 3);
	lv_obj_set_width(s_roller_min, 56);
	lv_obj_align(s_roller_min, LV_ALIGN_TOP_MID, 34, 20);

	/* Year roller */
	s_roller_year = lv_roller_create(s_cont_edit);
	lv_roller_set_options(s_roller_year, s_opts_year, LV_ROLLER_MODE_NORMAL);
	lv_roller_set_visible_row_count(s_roller_year, 3);
	lv_obj_set_width(s_roller_year, 62);
	lv_obj_align(s_roller_year, LV_ALIGN_TOP_MID, -53, 100);

	/* Dash between year and month */
	lv_obj_t *dash1 = lv_label_create(s_cont_edit);
	lv_label_set_text(dash1, "-");
	lv_obj_align(dash1, LV_ALIGN_TOP_MID, -19, 118);

	/* Month roller */
	s_roller_mon = lv_roller_create(s_cont_edit);
	lv_roller_set_options(s_roller_mon, s_opts_mon, LV_ROLLER_MODE_INFINITE);
	lv_roller_set_visible_row_count(s_roller_mon, 3);
	lv_obj_set_width(s_roller_mon, 48);
	lv_obj_align(s_roller_mon, LV_ALIGN_TOP_MID, 0, 100);

	/* Dash between month and day */
	lv_obj_t *dash2 = lv_label_create(s_cont_edit);
	lv_label_set_text(dash2, "-");
	lv_obj_align(dash2, LV_ALIGN_TOP_MID, 27, 118);

	/* Day roller */
	s_roller_day = lv_roller_create(s_cont_edit);
	lv_roller_set_options(s_roller_day, s_opts_day, LV_ROLLER_MODE_NORMAL);
	lv_roller_set_visible_row_count(s_roller_day, 3);
	lv_obj_set_width(s_roller_day, 48);
	lv_obj_align(s_roller_day, LV_ALIGN_TOP_MID, 53, 100);

	/* Cancel / OK buttons */
	make_btn(s_cont_edit, LV_SYMBOL_CLOSE " Cancel",
		 72, 36, LV_ALIGN_BOTTOM_LEFT,  20,  -16, cb_cancel_btn);
	make_btn(s_cont_edit, LV_SYMBOL_OK " OK",
		 72, 36, LV_ALIGN_BOTTOM_RIGHT, -20, -16, cb_ok_btn);

	/* 1-second timer, created paused — resumed only while page is active */
	s_timer = lv_timer_create(update_clock, 1000, NULL);
	lv_timer_pause(s_timer);

	return 0;
}

/* ── Page lifecycle ────────────────────────────────────────────────────── */

static void page_clock_enter(void)
{
	if (s_edit_mode) {
		/* defensive: shouldn't normally be true on re-entry */
		leave_edit_mode();
		return;
	}
	update_clock(NULL); /* show current time immediately on entry */
	lv_timer_resume(s_timer);
}

static void page_clock_leave(void)
{
	if (s_edit_mode) {
		leave_edit_mode(); /* implicit cancel; resumes timer momentarily */
	}
	lv_timer_pause(s_timer); /* always pause while not visible */
}

/* ── Page ops ──────────────────────────────────────────────────────────── */

const struct page_ops page_clock_ops = {
	.name         = "clock",
	.mouse_active = false,
	.create       = page_clock_create,
	.on_enter     = page_clock_enter,
	.on_leave     = page_clock_leave,
};
