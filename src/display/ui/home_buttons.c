/*
 * SPDX-License-Identifier: MIT
 *
 * Home screen circle button ring.
 *
 * Button layout is determined at compile time from the xiaord,home-buttons
 * devicetree node when present, or falls back to the built-in 12-button array.
 * Either way, 12 fixed clock positions are available; unassigned slots have no
 * LVGL object (s_circle_btn_objs[i] == NULL) and are silently skipped.
 *
 * Each button tap fires INPUT_VIRTUAL_POS_0 + position_index. The displayed
 * icon is a Unicode codepoint stored in symbol_cp, independent of the fired code.
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <dt-bindings/xiaord/input_codes.h>
#include <dt-bindings/xiaord/icons.h>
#include "page_iface.h"
#include "display_api.h"
#include "ui_btn.h"
#include "icon_utf8.h"
#include "home_buttons.h"

/* ── Button descriptor ─────────────────────────────────────────────────── */

struct circle_btn_desc {
	uint32_t symbol_cp; /* Unicode codepoint for icon */
	int8_t   nav_page;  /* >=0: navigate on tap; -1: no nav */
	bool     dismiss;   /* true: hide buttons on tap */
	bool     active;    /* true if this slot is populated */
};

/* ── Compile-time button table ─────────────────────────────────────────── */

#if DT_HAS_COMPAT_STATUS_OKAY(xiaord_home_buttons)

#define HOME_BTN_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(xiaord_home_buttons)

#define BTN_NAV(node) \
	COND_CODE_1(DT_NODE_HAS_PROP(node, nav_page), \
		(DT_PROP(node, nav_page)), (-1))

#define BTN_INIT(node) \
	[DT_PROP(node, position)] = { \
		.symbol_cp = DT_PROP(node, symbol), \
		.nav_page  = BTN_NAV(node), \
		.dismiss   = DT_PROP_OR(node, dismiss, false), \
		.active    = true, \
	},

static const struct circle_btn_desc s_circle_btns[12] = {
	DT_FOREACH_CHILD(HOME_BTN_NODE, BTN_INIT)
};

#else /* fallback: built-in 12-button layout */

static const struct circle_btn_desc s_circle_btns[12] = {
	{ ICON_UPLOAD,     -1,         false, true },
	{ ICON_IMAGE,      -1,         false, true },
	{ ICON_VOLUME_MAX, -1,         false, true },
	{ ICON_MUTE,       -1,         false, true },
	{ ICON_VOLUME_MID, -1,         false, true },
	{ ICON_NEXT,       -1,         false, true },
	{ ICON_PLAY,       -1,         false, true },
	{ ICON_PREV,       -1,         false, true },
	{ ICON_WARNING,    -1,         false, true },
	{ ICON_USB,        -1,         false, true },
	{ ICON_BLUETOOTH,  PAGE_BT,    false, true },
	{ ICON_SETTINGS,   PAGE_CLOCK, false, true },
};

#endif /* DT_HAS_COMPAT_STATUS_OKAY(xiaord_home_buttons) */

/* ── Widget state ──────────────────────────────────────────────────────── */

static lv_obj_t   *s_circle_btn_objs[12];
static bool        s_btns_visible;

static lv_timer_t *s_repeat_timer;
static int         s_repeat_idx;
static bool        s_repeat_fired;
static lv_timer_t *s_autohide_timer;
static lv_timer_t *s_tap_timer;
static bool        s_tap_pending;
static bool        s_suppress_click;
static bool        s_knob_active;
static bool        s_knob_fired;
static lv_point_t  s_knob_prev;
static int32_t     s_knob_accum;

#define CENTER_DISMISS_RADIUS 70
#define KNOB_MIN_RADIUS 32
#define KNOB_STEP_CROSS 1800
#define DOUBLE_TAP_MS CONFIG_XIAORD_DOUBLE_TAP_MS

/* ── Timers ────────────────────────────────────────────────────────────── */

static int32_t point_radius_sq(const lv_point_t *p)
{
    int32_t dx = p->x - 120;
    int32_t dy = p->y - 120;
    return dx * dx + dy * dy;
}

static bool point_in_center(const lv_point_t *p)
{
    return point_radius_sq(p) <= CENTER_DISMISS_RADIUS * CENTER_DISMISS_RADIUS;
}

static bool point_on_knob_ring(const lv_point_t *p)
{
    return point_radius_sq(p) >= KNOB_MIN_RADIUS * KNOB_MIN_RADIUS;
}

static void home_buttons_run_legacy_tap(const lv_point_t *p)
{
    if (!s_btns_visible) {
        home_buttons_set_visible(true);
        return;
    }
    if (point_in_center(p)) {
        home_buttons_set_visible(false);
    }
}

static void tap_timer_cb(lv_timer_t *t)
{
    ARG_UNUSED(t);
    lv_timer_pause(s_tap_timer);
    if (!s_tap_pending) {
        return;
    }
    s_tap_pending = false;
    ss_fire_behavior(INPUT_VIRTUAL_GESTURE_TAP);
}

static void knob_handle_point(const lv_point_t *p)
{
    if (!point_on_knob_ring(p)) {
        return;
    }

    int32_t prev_x = s_knob_prev.x - 120;
    int32_t prev_y = s_knob_prev.y - 120;
    int32_t cur_x = p->x - 120;
    int32_t cur_y = p->y - 120;
    int32_t cross = prev_x * cur_y - prev_y * cur_x;

    if (cross > -80 && cross < 80) {
        s_knob_prev = *p;
        return;
    }

    s_knob_accum += cross;
    s_knob_prev = *p;

    while (s_knob_accum >= KNOB_STEP_CROSS) {
        ss_fire_behavior(INPUT_VIRTUAL_GESTURE_CW);
        s_knob_accum -= KNOB_STEP_CROSS;
        s_knob_fired = true;
    }
    while (s_knob_accum <= -KNOB_STEP_CROSS) {
        ss_fire_behavior(INPUT_VIRTUAL_GESTURE_CCW);
        s_knob_accum += KNOB_STEP_CROSS;
        s_knob_fired = true;
    }
}
static void autohide_timer_cb(lv_timer_t *t)
{
	ARG_UNUSED(t);
	home_buttons_set_visible(false);
}

static void repeat_timer_cb(lv_timer_t *t)
{
	ARG_UNUSED(t);
	ss_fire_behavior(INPUT_VIRTUAL_POS_0 + s_repeat_idx);
	if (!s_repeat_fired) {
		s_repeat_fired = true;
		lv_timer_set_period(t, 60);
	}
}

/* ── Button event callback ─────────────────────────────────────────────── */

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
			ss_fire_behavior(INPUT_VIRTUAL_POS_0 + idx);
			if (s_circle_btns[idx].nav_page >= 0)
				ss_navigate_to(s_circle_btns[idx].nav_page);
			if (s_circle_btns[idx].dismiss)
				home_buttons_set_visible(false);
		}
	}
}

/* ── Tap overlay callback ──────────────────────────────────────────────── */

static void tap_overlay_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    lv_point_t p = {0};

    if (indev) {
        lv_indev_get_point(indev, &p);
    }

    if (code == LV_EVENT_PRESSED) {
        s_knob_prev = p;
        s_knob_accum = 0;
        s_knob_fired = false;
        s_knob_active = point_on_knob_ring(&p);
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        if (s_knob_active) {
            knob_handle_point(&p);
        }
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (s_knob_fired) {
            s_suppress_click = true;
        }
        s_knob_active = false;
        return;
    }

    if (code != LV_EVENT_CLICKED) {
        return;
    }

    if (s_suppress_click) {
        s_suppress_click = false;
        return;
    }

    if (s_tap_pending) {
        s_tap_pending = false;
        lv_timer_pause(s_tap_timer);
        home_buttons_run_legacy_tap(&p);
        return;
    }

    s_tap_pending = true;
    lv_timer_reset(s_tap_timer);
    lv_timer_resume(s_tap_timer);
}
void home_buttons_set_visible(bool visible)
{
	s_btns_visible = visible;
	for (int i = 0; i < 12; i++) {
		if (s_circle_btn_objs[i] == NULL)
			continue;
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
		lv_timer_pause(s_tap_timer);
		s_tap_pending = false;
	}
}

void home_buttons_pause(void)
{
	lv_timer_pause(s_repeat_timer);
	lv_timer_pause(s_autohide_timer);
	lv_timer_pause(s_tap_timer);
	s_tap_pending = false;
}

void home_buttons_create(lv_obj_t *parent)
{
	/* Tap overlay — full-tile, behind buttons */
	lv_obj_t *overlay = lv_obj_create(parent);
	lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
	lv_obj_align(overlay, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(overlay, 0, 0);
	lv_obj_set_scrollbar_mode(overlay, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(overlay, tap_overlay_cb, LV_EVENT_ALL, NULL);

	/* Circle button ring — 12 slots, sparse */
	int16_t pos[12][2];
	ui_circle_12_positions(pos, UI_CIRCLE_LAYOUT_RADIUS);
	s_btns_visible = false;

	static char icon_bufs[12][5]; /* UTF-8: max 4 bytes + null */

	for (int i = 0; i < 12; i++) {
		if (!s_circle_btns[i].active) {
			s_circle_btn_objs[i] = NULL;
			continue;
		}
		unicode_to_utf8(s_circle_btns[i].symbol_cp, icon_bufs[i]);
		lv_obj_t *btn = ui_create_circle_btn(parent, icon_bufs[i],
						      pos[i][0], pos[i][1],
						      circle_btn_cb,
						      (void *)(uintptr_t)i);
		lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
		s_circle_btn_objs[i] = btn;
	}

	s_repeat_timer = lv_timer_create(repeat_timer_cb, 1000, NULL);
	lv_timer_pause(s_repeat_timer);

	s_autohide_timer = lv_timer_create(autohide_timer_cb, 10000, NULL);
	lv_timer_pause(s_autohide_timer);

	s_tap_timer = lv_timer_create(tap_timer_cb, DOUBLE_TAP_MS, NULL);
	lv_timer_pause(s_tap_timer);
}
