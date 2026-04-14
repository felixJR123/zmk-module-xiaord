/*
 * SPDX-License-Identifier: MIT
 *
 * Xiaord display coordinator — independent-screen multi-page system.
 *
 * Responsibilities:
 *  - Create independent LVGL screens, register all pages
 *  - Manage page lifecycle (on_enter / on_leave) on transitions
 *  - Provide ss_navigate_to() and ss_fire_behavior() APIs for pages to call
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/led.h>
#include <lvgl.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "page_iface.h"
#include "display_api.h"

extern const lv_image_dsc_t img_bg;

static const struct device *status_display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const struct device *status_backlight = DEVICE_DT_GET(DT_NODELABEL(display_backlight));
static struct k_timer status_screen_idle_timer;
static bool status_screen_is_blank;
#define STATUS_SCREEN_IDLE_TIMEOUT_MS CONFIG_ZMK_IDLE_TIMEOUT

static void status_screen_set_backlight(bool on)
{
    if (!device_is_ready(status_backlight)) {
        LOG_WRN("backlight device not ready");
        return;
    }

    int err = led_set_brightness(status_backlight, 0, on ? 255 : 0);
    if (err) {
        LOG_WRN("backlight brightness failed: %d", err);
        if (on) {
            led_on(status_backlight, 0);
        } else {
            led_off(status_backlight, 0);
        }
    }
}

static void status_screen_set_blank(bool blank)
{
    if (device_is_ready(status_display)) {
        if (blank) {
            display_blanking_on(status_display);
        } else {
            display_blanking_off(status_display);
        }
    }

    status_screen_set_backlight(!blank);
    status_screen_is_blank = blank;
}

static void status_screen_idle_timeout(struct k_timer *timer)
{
    ARG_UNUSED(timer);
    status_screen_set_blank(true);
}

static void status_screen_restart_idle_timer(void)
{
    status_screen_set_blank(false);
    k_timer_start(&status_screen_idle_timer, K_MSEC(STATUS_SCREEN_IDLE_TIMEOUT_MS), K_NO_WAIT);
}

static int status_screen_keycode_state_changed_listener(const zmk_event_t *eh)
{
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (!ev || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    status_screen_restart_idle_timer();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(status_screen_listener, status_screen_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(status_screen_listener, zmk_keycode_state_changed);
BUILD_ASSERT(IS_ENABLED(CONFIG_ZMK_VIRTUAL_KEY_SOURCE),
	"xiaord status_screen requires CONFIG_ZMK_VIRTUAL_KEY_SOURCE");
BUILD_ASSERT(IS_ENABLED(CONFIG_LV_USE_THEME_DEFAULT),
	"xiaord status_screen requires CONFIG_LV_USE_THEME_DEFAULT");

/* ── Page declarations ─────────────────────────────────────────────────── */

extern const struct page_ops page_home_ops;
extern const struct page_ops page_clock_ops;
extern const struct page_ops page_bt_ops;

/* ── Virtual key source device ─────────────────────────────────────────── */

static const struct device *s_vkey = DEVICE_DT_GET(DT_NODELABEL(virtual_key_source));

/* ── Page registration table ───────────────────────────────────────────── */

struct page_entry {
	const struct page_ops *ops;
	lv_obj_t *screen; /* independent screen created with lv_obj_create(NULL) */
};

static struct page_entry s_pages[] = {
	[PAGE_HOME]  = { .ops = &page_home_ops },
	[PAGE_CLOCK] = { .ops = &page_clock_ops },
	[PAGE_BT]    = { .ops = &page_bt_ops },
};

#define PAGE_COUNT ARRAY_SIZE(s_pages)

/* ── Active page tracking ──────────────────────────────────────────────── */

static uint8_t s_active_page;

/* ── Public API ─────────────────────────────────────────────────────────── */

void ss_navigate_to(uint8_t page_idx)
{
	if (page_idx >= PAGE_COUNT || page_idx == s_active_page) {
		return;
	}

	/* Leave current page */
	struct page_entry *old = &s_pages[s_active_page];
	if (old->ops->on_leave) {
		old->ops->on_leave();
	}

	/* Enter new page */
	s_active_page = page_idx;
	lv_scr_load(s_pages[page_idx].screen);
	if (s_pages[page_idx].ops->on_enter) {
		s_pages[page_idx].ops->on_enter();
	}


}

void ss_fire_behavior(input_virtual_code code)
{
	input_report(s_vkey, INPUT_EV_ZMK_BEHAVIORS, code, 1, true, K_NO_WAIT);
	input_report(s_vkey, INPUT_EV_ZMK_BEHAVIORS, code, 0, true, K_NO_WAIT);
}

/* ── Color theme ─────────────────────────────────────────────────────────── */

static void xiaord_initialize_color_theme(void)
{
	lv_display_t *disp = lv_display_get_default();

	/* Hardware rotation: called here (after LVGL init) so disp_data->cap is
	 * captured with NORMAL orientation, letting lv_display_set_rotation below
	 * handle touch coordinate transformation without Zephyr driver interference. */
	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (device_is_ready(display_dev)) {
		display_set_orientation(display_dev, DISPLAY_ORIENTATION_ROTATED_270);
	}

	/* Touch coordinate transformation via LVGL rotation. */
	lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);

	lv_theme_t *theme = lv_theme_default_init(
		disp,
		lv_palette_main(LV_PALETTE_BLUE),
		lv_palette_main(LV_PALETTE_TEAL),
		true,                    /* dark mode */
		&lv_font_montserrat_16   /* default font */
	);
	if (theme) {
		lv_display_set_theme(disp, theme);
	}
}

/* ── Entry point called by ZMK display subsystem ───────────────────────── */

lv_obj_t *zmk_display_status_screen(void)
{
    xiaord_initialize_color_theme();

	/* Create an independent screen for each page */
	for (size_t i = 0; i < PAGE_COUNT; i++) {
		lv_obj_t *screen = lv_obj_create(NULL);
		lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_set_style_bg_image_src(screen, &img_bg, LV_PART_MAIN);
		s_pages[i].screen = screen;

		/* Build page widgets */
		if (s_pages[i].ops->create) {
			s_pages[i].ops->create(screen);
		}

	}

	/* Fire on_enter for the initial page */
	if (s_pages[0].ops->on_enter) {
		s_pages[0].ops->on_enter();
	}

	k_timer_init(&status_screen_idle_timer, status_screen_idle_timeout, NULL);
	k_timer_start(&status_screen_idle_timer, K_MSEC(STATUS_SCREEN_IDLE_TIMEOUT_MS), K_NO_WAIT);

	return s_pages[0].screen;
}
