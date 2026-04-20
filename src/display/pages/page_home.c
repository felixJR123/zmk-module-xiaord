/*
 * SPDX-License-Identifier: MIT
 *
 * Home screen: date/time labels (upper half) + peripheral battery arc gauges
 * (lower half) + icon buttons (delegated to home_buttons.c).
 */

#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>
#include <lvgl.h>
#include <zmk/split/central.h>
#include "page_iface.h"
#include "display_api.h"
#include "endpoint_status.h"
#include "battery_status.h"
#include "home_buttons.h"

/* ── RTC device ────────────────────────────────────────────────────────── */

#if !IS_ENABLED(CONFIG_XIAORD_REMOVE_DATE_TIME)
static const struct device *s_rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
#endif

/* ── Widget handles ────────────────────────────────────────────────────── */

#if !IS_ENABLED(CONFIG_XIAORD_REMOVE_DATE_TIME)
static lv_obj_t   *s_date_lbl;
static lv_obj_t   *s_time_lbl;
static lv_timer_t *s_timer;
#endif
static lv_obj_t   *s_output_lbl;
static lv_obj_t   *s_periph_bat_arcs[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
static lv_obj_t   *s_periph_bat_lbls[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
static bool        s_info_visible = true;

/* ── Endpoint status callback ──────────────────────────────────────────── */

static void home_endpoint_cb(struct endpoint_state state)
{
	endpoint_status_update_label(s_output_lbl, state);
}

static void set_obj_visible(lv_obj_t *obj, bool visible)
{
	if (!obj) {
		return;
	}
	if (visible) {
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
	}
}

void page_home_set_info_visible(bool visible)
{
	s_info_visible = visible;
#if !IS_ENABLED(CONFIG_XIAORD_REMOVE_DATE_TIME)
	set_obj_visible(s_date_lbl, visible);
	set_obj_visible(s_time_lbl, visible);
#endif
	set_obj_visible(s_output_lbl, visible);

	for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; i++) {
		set_obj_visible(s_periph_bat_arcs[i], visible);
		set_obj_visible(s_periph_bat_lbls[i], visible);
	}
}

bool page_home_toggle_info_visible(void)
{
	page_home_set_info_visible(!s_info_visible);
	return s_info_visible;
}

bool page_home_info_visible(void)
{
	return s_info_visible;
}

/* ── Month / weekday name tables ───────────────────────────────────────── */

#if !IS_ENABLED(CONFIG_XIAORD_REMOVE_DATE_TIME)
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

/* ── Page create ───────────────────────────────────────────────────────── */

#endif

static int page_home_create(lv_obj_t *tile)
{
	/* ── Date label — upper area ────────────────────────────────────── */
#if !IS_ENABLED(CONFIG_XIAORD_REMOVE_DATE_TIME)
	s_date_lbl = lv_label_create(tile);
	lv_label_set_text(s_date_lbl, "--- -- ---");
	lv_obj_align(s_date_lbl, LV_ALIGN_CENTER, 0, -67);

	/* ── Time label ─────────────────────────────────────────────────── */
	s_time_lbl = lv_label_create(tile);
	lv_label_set_text(s_time_lbl, "--:--");
	lv_obj_set_style_text_font(s_time_lbl, &lv_font_montserrat_48, 0);
	lv_obj_align(s_time_lbl, LV_ALIGN_CENTER, 0, -27);
#endif

	/* ── Output status label ────────────────────────────────────────── */
	s_output_lbl = create_output_status_label(tile, &lv_font_montserrat_16);
	lv_obj_align(s_output_lbl, LV_ALIGN_BOTTOM_MID, 0, -37);

	/* ── Peripheral battery arc gauges — lower half ─────────────────── */
	const int n_periph     = ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT;
	const int spacing      = 74;
	const int arc_sz       = 48;
	const int center_y_off = 34;

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

		s_periph_bat_arcs[i] = arc;
		s_periph_bat_lbls[i] = lbl;
	}

	endpoint_status_register_cb(home_endpoint_cb);
	battery_status_init(s_periph_bat_arcs, s_periph_bat_lbls);

	/* ── Button ring ─────────────────────────────────────────────────── */
	home_buttons_create(tile);
	page_home_set_info_visible(s_info_visible);
#if !IS_ENABLED(CONFIG_XIAORD_REMOVE_DATE_TIME)

	/* 1-second timer, created paused — resumed only while page is active */
	s_timer = lv_timer_create(update_datetime, 1000, NULL);
	lv_timer_pause(s_timer);
#endif

	return 0;
}

/* ── Page lifecycle ────────────────────────────────────────────────────── */

static void page_home_enter(void)
{
#if !IS_ENABLED(CONFIG_XIAORD_REMOVE_DATE_TIME)
	update_datetime(NULL); /* show current time immediately on entry */
	lv_timer_resume(s_timer);
#endif
	home_buttons_set_visible(false);
}

static void page_home_leave(void)
{
#if !IS_ENABLED(CONFIG_XIAORD_REMOVE_DATE_TIME)
	lv_timer_pause(s_timer);
#endif
	home_buttons_pause();
}

/* ── Page ops ──────────────────────────────────────────────────────────── */

const struct page_ops page_home_ops = {
	.name         = "home",
	.create       = page_home_create,
	.on_enter     = page_home_enter,
	.on_leave     = page_home_leave,
};
