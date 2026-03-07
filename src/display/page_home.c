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
#include "bt_status.h"
#include "ui_btn.h"

/* ── RTC device ────────────────────────────────────────────────────────── */

static const struct device *s_rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

/* ── Widget handles ────────────────────────────────────────────────────── */

static lv_obj_t   *s_date_lbl;
static lv_obj_t   *s_time_lbl;
static lv_timer_t *s_timer;

static lv_obj_t   *s_circle_btn_objs[12];
static bool        s_btns_visible;

static lv_timer_t *s_repeat_timer;   /* long-press repeat timer */
static int         s_repeat_idx;     /* index of button being held */
static bool        s_repeat_fired;   /* true after first repeat fire */
static lv_timer_t *s_autohide_timer; /* 10s inactivity auto-hide */

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

/* ── Circle button descriptor ──────────────────────────────────────────── */

struct circle_btn_desc {
	int16_t     x, y;       /* offset from center */
	const char *symbol;
	uint16_t    code;       /* SS_* code for ss_fire_behavior */
	int8_t      nav_page;   /* >=0: also navigate; -1: no nav */
};

static const struct circle_btn_desc s_circle_btns[12] = {
	{   0, -105, LV_SYMBOL_UPLOAD,     INPUT_VIRTUAL_SYM_UPLOAD,     -1         },
	{  53,  -91, LV_SYMBOL_POWER,      INPUT_VIRTUAL_SYM_POWER,      -1         },
	{  91,  -53, LV_SYMBOL_VOLUME_MAX, INPUT_VIRTUAL_SYM_VOLUME_MAX, -1         },
	{ 105,    0, LV_SYMBOL_MUTE,       INPUT_VIRTUAL_SYM_MUTE,       -1         },
	{  91,   53, LV_SYMBOL_VOLUME_MID, INPUT_VIRTUAL_SYM_VOLUME_MID, -1         },
	{  53,   91, LV_SYMBOL_PLUS,       INPUT_VIRTUAL_SYM_PLUS,       -1         },
	{   0,  105, LV_SYMBOL_MINUS,      INPUT_VIRTUAL_SYM_MINUS,      -1         },
	{ -53,   91, LV_SYMBOL_EYE_CLOSE,  INPUT_VIRTUAL_SYM_EYE_CLOSE,  -1         },
	{ -91,   53, LV_SYMBOL_USB,        INPUT_VIRTUAL_SYM_USB,        -1         },
	{-105,    0, LV_SYMBOL_BLUETOOTH,  INPUT_VIRTUAL_SYM_BLUETOOTH,  PAGE_BT    },
	{ -91,  -53, LV_SYMBOL_HOME,       INPUT_VIRTUAL_SYM_HOME,       -1         },
	{ -53,  -91, LV_SYMBOL_SETTINGS,   INPUT_VIRTUAL_SYM_SETTINGS,   PAGE_CLOCK },
};

/* ── Show/hide helper ──────────────────────────────────────────────────── */

#define CENTER_DISMISS_RADIUS 70  /* px — inner edge of buttons is at r=89 */

static void set_btns_visible(bool visible); /* forward declaration */

static void autohide_timer_cb(lv_timer_t *t)
{
	ARG_UNUSED(t);
	set_btns_visible(false);
}

static void set_btns_visible(bool visible)
{
	s_btns_visible = visible;
	for (int i = 0; i < 12; i++) {
		if (visible)
			lv_obj_clear_flag(s_circle_btn_objs[i], LV_OBJ_FLAG_HIDDEN);
		else
			lv_obj_add_flag(s_circle_btn_objs[i], LV_OBJ_FLAG_HIDDEN);
	}
	if (visible) {
		lv_timer_reset(s_autohide_timer);
		lv_timer_resume(s_autohide_timer);
	} else {
		lv_timer_pause(s_autohide_timer);
		lv_timer_pause(s_repeat_timer);
	}
}

/* ── Tap overlay callback ──────────────────────────────────────────────── */

static void tap_overlay_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	if (!s_btns_visible) {
		set_btns_visible(true);
		return;
	}
	/* Buttons visible — dismiss if tap is near center */
	lv_point_t p;
	lv_indev_get_point(lv_indev_active(), &p);
	int dx = p.x - 120, dy = p.y - 120;
	if (dx * dx + dy * dy <= CENTER_DISMISS_RADIUS * CENTER_DISMISS_RADIUS)
		set_btns_visible(false);
}

/* ── Circle button callback ────────────────────────────────────────────── */

static void repeat_timer_cb(lv_timer_t *t)
{
	ARG_UNUSED(t);
	ss_fire_behavior(s_circle_btns[s_repeat_idx].code);
	if (!s_repeat_fired) {
		s_repeat_fired = true;
		lv_timer_set_period(t, 60); /* switch from 1s initial to 60ms repeat */
	}
}

static void circle_btn_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	int idx = (int)(uintptr_t)lv_event_get_user_data(e);

	if (code == LV_EVENT_PRESSED) {
		s_repeat_idx = idx;
		s_repeat_fired = false;
		lv_timer_set_period(s_repeat_timer, 1000);
		lv_timer_reset(s_repeat_timer);
		lv_timer_resume(s_repeat_timer);
		lv_timer_reset(s_autohide_timer);
		return;
	}
	if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
		lv_timer_pause(s_repeat_timer);
		if (!s_repeat_fired) {
			/* Tap — fire once */
			ss_fire_behavior(s_circle_btns[idx].code);
			if (s_circle_btns[idx].nav_page >= 0)
				ss_navigate_to(s_circle_btns[idx].nav_page);
			if (s_circle_btns[idx].code == INPUT_VIRTUAL_SYM_HOME)
				set_btns_visible(false);
		}
	}
}

/* ── Page create ───────────────────────────────────────────────────────── */

static int page_home_create(lv_obj_t *tile)
{
	/* ── Date label — upper area ────────────────────────────────────── */
	s_date_lbl = lv_label_create(tile);
	lv_label_set_text(s_date_lbl, "--- -- ---");
	lv_obj_align(s_date_lbl, LV_ALIGN_CENTER, 0, -67);

	/* ── Time label ─────────────────────────────────────────────────── */
	s_time_lbl = lv_label_create(tile);
	lv_label_set_text(s_time_lbl, "--:--");
	lv_obj_set_style_text_font(s_time_lbl, &lv_font_montserrat_48, 0);
	lv_obj_align(s_time_lbl, LV_ALIGN_CENTER, 0, -27);

	/* ── Output status label ────────────────────────────────────────── */
	lv_obj_t *output_lbl = create_output_status_label(tile, &lv_font_montserrat_16);
	lv_obj_align(output_lbl, LV_ALIGN_BOTTOM_MID, 0, -35);

	/* ── Peripheral battery arc gauges — lower half ─────────────────── */
	lv_obj_t *periph_bat_arcs[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
	lv_obj_t *periph_bat_lbls[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];

	const int n_periph     = ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT;
	const int spacing      = 70;
	const int arc_sz       = 48;
	const int center_y_off = 40;

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

	/* ── Tap overlay — full-tile, behind buttons ────────────────────── */
	lv_obj_t *overlay = lv_obj_create(tile);
	lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
	lv_obj_align(overlay, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(overlay, 0, 0);
	lv_obj_set_scrollbar_mode(overlay, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(overlay, tap_overlay_cb, LV_EVENT_ALL, NULL);

	/* ── Circle button ring — 12 buttons on r=105px circumference ──── */
	s_btns_visible = false;
	for (int i = 0; i < 12; i++) {
		const struct circle_btn_desc *d = &s_circle_btns[i];
		lv_obj_t *btn = ui_create_circle_btn(tile, d->symbol, d->x, d->y,
						      circle_btn_cb, (void *)(uintptr_t)i);
		lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
		s_circle_btn_objs[i] = btn;
	}

	/* 1-second timer, created paused — resumed only while page is active */
	s_timer = lv_timer_create(update_datetime, 1000, NULL);
	lv_timer_pause(s_timer);

	s_repeat_timer = lv_timer_create(repeat_timer_cb, 1000, NULL);
	lv_timer_pause(s_repeat_timer);

	s_autohide_timer = lv_timer_create(autohide_timer_cb, 10000, NULL);
	lv_timer_pause(s_autohide_timer);

	return 0;
}

/* ── Page lifecycle ────────────────────────────────────────────────────── */

static void page_home_enter(void)
{
	update_datetime(NULL); /* show current time immediately on entry */
	lv_timer_resume(s_timer);
	set_btns_visible(false);
}

static void page_home_leave(void)
{
	lv_timer_pause(s_timer);
	lv_timer_pause(s_repeat_timer);
	lv_timer_pause(s_autohide_timer);
}

/* ── Page ops ──────────────────────────────────────────────────────────── */

const struct page_ops page_home_ops = {
	.name         = "home",
	.create       = page_home_create,
	.on_enter     = page_home_enter,
	.on_leave     = page_home_leave,
};
