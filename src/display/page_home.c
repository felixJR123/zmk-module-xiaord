/*
 * SPDX-License-Identifier: MIT
 *
 * Home screen: date/time labels (upper half) + peripheral battery arc gauges
 * (lower half) + icon buttons (bottom row).
 */

#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <lvgl.h>
#include <zmk/split/central.h>
#include <dt-bindings/xiaord/input_codes.h>
#include "page_ops.h"
#include "home_status.h"

/* ── RTC device ────────────────────────────────────────────────────────── */

static const struct device *s_rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

/* ── Widget handles ────────────────────────────────────────────────────── */

static lv_obj_t   *s_date_lbl;
static lv_obj_t   *s_time_lbl;
static lv_timer_t *s_timer;

/* ── Month / weekday name tables ───────────────────────────────────────── */

static const char *month_names[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char *day_names[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

/* ── Timer callback ────────────────────────────────────────────────────── */

static void update_datetime(lv_timer_t *t)
{
	ARG_UNUSED(t);

	struct rtc_time time = {};

	if (rtc_get_time(s_rtc, &time) != 0) {
		return;
	}

	/* "Jan 01 Mon" */
	lv_label_set_text_fmt(s_date_lbl, "%s %02d %s",
			      month_names[time.tm_mon],
			      time.tm_mday,
			      day_names[time.tm_wday]);

	/* "23:59" */
	lv_label_set_text_fmt(s_time_lbl, "%02d:%02d",
			      time.tm_hour, time.tm_min);
}

/* ── Button callbacks ──────────────────────────────────────────────────── */

static void bootloader_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		ss_fire_behavior(ZMK_BEHAVIOR_BOOTLOADER);
	}
}

static void datetime_area_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED) {
		ss_navigate_to(PAGE_CLOCK);
	}
}

static void lower_area_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED) {
		ss_navigate_to(PAGE_MACROPAD);
	}
}

/* ── Page create ───────────────────────────────────────────────────────── */

static int page_home_create(lv_obj_t *tile)
{
	/* ── Date label — upper area ────────────────────────────────────── */
	s_date_lbl = lv_label_create(tile);
	lv_label_set_text(s_date_lbl, "--- -- ---");
	lv_obj_align(s_date_lbl, LV_ALIGN_CENTER, 0, -77);

	/* ── Time label ─────────────────────────────────────────────────── */
	s_time_lbl = lv_label_create(tile);
	lv_label_set_text(s_time_lbl, "--:--");
	lv_obj_set_style_text_font(s_time_lbl, &lv_font_montserrat_48, 0);
	lv_obj_align(s_time_lbl, LV_ALIGN_CENTER, 0, -37);

	/* ── Output status label ────────────────────────────────────────── */
	lv_obj_t *output_lbl = lv_label_create(tile);
	lv_label_set_text(output_lbl, "");
	lv_obj_align(output_lbl, LV_ALIGN_BOTTOM_MID, 0, -20);

	/* ── Peripheral battery arc gauges — lower half ─────────────────── */
	lv_obj_t *periph_bat_arcs[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
	lv_obj_t *periph_bat_lbls[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];

	const int n_periph     = ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT;
	const int spacing      = 70;
	const int arc_sz       = 48;
	const int center_y_off = 55;

	for (int i = 0; i < n_periph; i++) {
		/* X offset: centres the group symmetrically around x=0 */
		int x = (int)((i - (n_periph - 1) / 2.0f) * spacing);

		/* Arc widget */
		lv_obj_t *arc = lv_arc_create(tile);
		lv_obj_set_size(arc, arc_sz, arc_sz);
		lv_arc_set_range(arc, 0, 100);
		lv_arc_set_value(arc, 0);
		lv_arc_set_rotation(arc, 270);    /* start sweep at 12 o'clock */
		lv_arc_set_bg_angles(arc, 0, 360); /* full circle background */
		lv_arc_set_angles(arc, 0, 0);     /* value arc starts empty */

		/* Hide interactive knob */
		lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
		lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

		/* Background arc: dim */
		lv_obj_set_style_arc_color(arc, lv_color_hex(0x333333), LV_PART_MAIN);
		lv_obj_set_style_arc_width(arc, 4, LV_PART_MAIN);

		/* Indicator (value) arc: white */
		lv_obj_set_style_arc_color(arc, lv_color_white(), LV_PART_INDICATOR);
		lv_obj_set_style_arc_width(arc, 4, LV_PART_INDICATOR);

		/* Transparent background */
		lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, 0);

		lv_obj_align(arc, LV_ALIGN_CENTER, x, center_y_off);

		/* Percentage label inside the arc */
		lv_obj_t *lbl = lv_label_create(tile);
		lv_label_set_text(lbl, "--");
		lv_obj_align(lbl, LV_ALIGN_CENTER, x, center_y_off);

		periph_bat_arcs[i] = arc;
		periph_bat_lbls[i] = lbl;
	}

	home_status_init(output_lbl, periph_bat_arcs, periph_bat_lbls);

	/* ── Bootloader button — above date ────────────────────────────── */
	lv_obj_t *boot_btn = lv_obj_create(tile);
	lv_obj_set_size(boot_btn, 48, 48);
	lv_obj_align(boot_btn, LV_ALIGN_CENTER, 0, -102);
	lv_obj_set_style_bg_opa(boot_btn, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(boot_btn, 0, 0);
	lv_obj_add_flag(boot_btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(boot_btn, bootloader_btn_cb, LV_EVENT_ALL, NULL);

	lv_obj_t *boot_lbl = lv_label_create(boot_btn);
	lv_label_set_text(boot_lbl, LV_SYMBOL_UPLOAD);
	lv_obj_center(boot_lbl);

	/* ── DateTime overlay — long-press navigates to clock/RTC screen ── */
	lv_obj_t *datetime_overlay = lv_obj_create(tile);
	lv_obj_set_size(datetime_overlay, 200, 70);
	lv_obj_align(datetime_overlay, LV_ALIGN_CENTER, 0, -57);
	lv_obj_set_style_bg_opa(datetime_overlay, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(datetime_overlay, 0, 0);
	lv_obj_add_flag(datetime_overlay, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(datetime_overlay, datetime_area_cb, LV_EVENT_ALL, NULL);

	/* ── Lower overlay — long-press navigates to macropad screen ─────── */
	lv_obj_t *lower_overlay = lv_obj_create(tile);
	lv_obj_set_size(lower_overlay, 200, 100);
	lv_obj_align(lower_overlay, LV_ALIGN_CENTER, 0, 60);
	lv_obj_set_style_bg_opa(lower_overlay, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(lower_overlay, 0, 0);
	lv_obj_add_flag(lower_overlay, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(lower_overlay, lower_area_cb, LV_EVENT_ALL, NULL);

	/* 1-second timer, created paused — resumed only while page is active */
	s_timer = lv_timer_create(update_datetime, 1000, NULL);
	lv_timer_pause(s_timer);

	return 0;
}

/* ── Page lifecycle ────────────────────────────────────────────────────── */

static void page_home_enter(void)
{
	update_datetime(NULL); /* show current time immediately on entry */
	lv_timer_resume(s_timer);
}

static void page_home_leave(void)
{
	lv_timer_pause(s_timer);
}

/* ── Page ops ──────────────────────────────────────────────────────────── */

const struct page_ops page_home_ops = {
	.name         = "home",
	.create       = page_home_create,
	.on_enter     = page_home_enter,
	.on_leave     = page_home_leave,
};
