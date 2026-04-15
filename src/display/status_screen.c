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
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/devicetree.h>
#include <lvgl.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/display.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define STATUS_BACKLIGHT_NODE DT_CHOSEN(zmk_display_led)

#if DT_NODE_EXISTS(STATUS_BACKLIGHT_NODE) && DT_NODE_HAS_PROP(STATUS_BACKLIGHT_NODE, gpios)
static const struct gpio_dt_spec status_backlight_gpio = GPIO_DT_SPEC_GET(STATUS_BACKLIGHT_NODE, gpios);
#define STATUS_BACKLIGHT_HAS_GPIO 1
#else
#define STATUS_BACKLIGHT_HAS_GPIO 0
#endif

#include "page_iface.h"
#include "display_api.h"

#define XIAORD_FAMILY_BG_ENABLED \
    (IS_ENABLED(CONFIG_XIAORD_BG_4) || IS_ENABLED(CONFIG_XIAORD_BG_5) || IS_ENABLED(CONFIG_XIAORD_BG_6))
#define XIAORD_ORIGINAL_BG_ENABLED \
    (IS_ENABLED(CONFIG_XIAORD_BG_1) || IS_ENABLED(CONFIG_XIAORD_BG_2) || IS_ENABLED(CONFIG_XIAORD_BG_3))

#if !XIAORD_FAMILY_BG_ENABLED && !XIAORD_ORIGINAL_BG_ENABLED
#error "At least one CONFIG_XIAORD_BG_* option must be enabled"
#endif

#if !XIAORD_FAMILY_BG_ENABLED
#if IS_ENABLED(CONFIG_XIAORD_BG_1)
extern const lv_image_dsc_t img_bg_1;
#endif
#if IS_ENABLED(CONFIG_XIAORD_BG_2)
extern const lv_image_dsc_t img_bg_2;
#endif
#if IS_ENABLED(CONFIG_XIAORD_BG_3)
extern const lv_image_dsc_t img_bg_3;
#endif
#endif
#if IS_ENABLED(CONFIG_XIAORD_BG_4)
extern const lv_image_dsc_t img_bg_4;
#endif
#if IS_ENABLED(CONFIG_XIAORD_BG_5)
extern const lv_image_dsc_t img_bg_5;
#endif
#if IS_ENABLED(CONFIG_XIAORD_BG_6)
extern const lv_image_dsc_t img_bg_6;
#endif

static const lv_image_dsc_t *const status_backgrounds[] = {
#if !XIAORD_FAMILY_BG_ENABLED
#if IS_ENABLED(CONFIG_XIAORD_BG_1)
    &img_bg_1,
#endif
#if IS_ENABLED(CONFIG_XIAORD_BG_2)
    &img_bg_2,
#endif
#if IS_ENABLED(CONFIG_XIAORD_BG_3)
    &img_bg_3,
#endif
#endif
#if IS_ENABLED(CONFIG_XIAORD_BG_4)
    &img_bg_4,
#endif
#if IS_ENABLED(CONFIG_XIAORD_BG_5)
    &img_bg_5,
#endif
#if IS_ENABLED(CONFIG_XIAORD_BG_6)
    &img_bg_6,
#endif
};
static uint8_t status_background_index;

static const struct device *status_display;
static const struct device *status_backlight;
static uint32_t status_backlight_led;
#define STATUS_BACKLIGHT_LABEL "DISPLAY_BACKLIGHT"

static struct k_timer status_screen_idle_timer;
static struct k_timer status_background_timer;
static bool status_screen_is_blank;
#define STATUS_SCREEN_IDLE_TIMEOUT_MS CONFIG_ZMK_IDLE_TIMEOUT

static void status_screen_init_backlight(void)
{
    if (status_backlight) {
        return;
    }

#if DT_NODE_EXISTS(DT_CHOSEN(zmk_display_led))
    status_backlight = DEVICE_DT_GET(DT_PARENT(DT_CHOSEN(zmk_display_led)));
    status_backlight_led = DT_NODE_CHILD_IDX(DT_CHOSEN(zmk_display_led));
    if (status_backlight && !device_is_ready(status_backlight)) {
        status_backlight = NULL;
    }
#endif

    if (!status_backlight) {
        status_backlight = device_get_binding(STATUS_BACKLIGHT_LABEL);
    }
}

static void status_screen_init_display(void)
{
    if (status_display) {
        return;
    }

#if DT_NODE_EXISTS(DT_CHOSEN(zephyr_display))
    status_display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (status_display && !device_is_ready(status_display)) {
        status_display = NULL;
    }
#endif
}

static const struct device *status_backlight_dev(void)
{
    if (!status_backlight) {
        status_screen_init_backlight();
    }

    if (status_backlight && device_is_ready(status_backlight)) {
        return status_backlight;
    }
    return NULL;
}

static bool status_backlight_gpio_ready(bool on)
{
#if STATUS_BACKLIGHT_HAS_GPIO
    static bool configured;

    if (!device_is_ready(status_backlight_gpio.port)) {
        LOG_WRN("backlight GPIO device not ready");
        return false;
    }

    if (!configured) {
        int err = gpio_pin_configure_dt(&status_backlight_gpio,
                                        on ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE);
        if (err) {
            LOG_WRN("backlight GPIO configure failed: %d", err);
            return false;
        }
        configured = true;
    }

    return true;
#else
    return false;
#endif
}

static void status_screen_set_backlight(bool on)
{
#if STATUS_BACKLIGHT_HAS_GPIO
    if (status_backlight_gpio_ready(on)) {
        int err = gpio_pin_set_dt(&status_backlight_gpio, on ? 1 : 0);
        if (err) {
            LOG_WRN("backlight GPIO update failed: %d", err);
        }
        return;
    }
#endif

    const struct device *dev = status_backlight_dev();
    if (!dev) {
        LOG_WRN("backlight device not ready");
        return;
    }

    int err = led_set_brightness(dev, status_backlight_led, on ? 100 : 0);
    if (err) {
        LOG_WRN("backlight update failed: %d", err);
        return;
    }

    err = on ? led_on(dev, status_backlight_led) : led_off(dev, status_backlight_led);
    if (err) {
        LOG_WRN("backlight switch failed: %d", err);
    }
}

static void status_screen_set_blank(bool blank)
{
    status_screen_init_display();

    if (status_display && device_is_ready(status_display)) {
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

static const struct device *s_vkey;

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

static const lv_image_dsc_t *status_screen_current_background(void)
{
    return status_backgrounds[status_background_index];
}

static void status_screen_apply_background(void)
{
    const lv_image_dsc_t *bg = status_screen_current_background();

    for (size_t i = 0; i < PAGE_COUNT; i++) {
        if (s_pages[i].screen) {
            lv_obj_set_style_bg_image_src(s_pages[i].screen, bg, LV_PART_MAIN);
        }
    }
}

static void status_background_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    status_background_index = (status_background_index + 1) % ARRAY_SIZE(status_backgrounds);
    status_screen_apply_background();
}

K_WORK_DEFINE(status_background_work, status_background_work_handler);

static void status_background_timer_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);

    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &status_background_work);
    }
}

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

static const struct device *status_vkey_dev(void)
{
    if (s_vkey) {
        return s_vkey;
    }

#if DT_NODE_EXISTS(DT_NODELABEL(virtual_key_source))
    s_vkey = DEVICE_DT_GET(DT_NODELABEL(virtual_key_source));
#endif
    return s_vkey;
}

void ss_fire_behavior(input_virtual_code code)
{
    const struct device *vkey = status_vkey_dev();
    if (!vkey) {
        LOG_WRN("virtual key source device not ready");
        return;
    }

    input_report(vkey, INPUT_EV_ZMK_BEHAVIORS, code, 1, true, K_NO_WAIT);
    input_report(vkey, INPUT_EV_ZMK_BEHAVIORS, code, 0, true, K_NO_WAIT);
}

/* ── Color theme ─────────────────────────────────────────────────────────── */

static void xiaord_initialize_color_theme(void)
{
	lv_display_t *disp = lv_display_get_default();

	/* Hardware rotation: called here (after LVGL init) so disp_data->cap is
	 * captured with NORMAL orientation, letting lv_display_set_rotation below
	 * handle touch coordinate transformation without Zephyr driver interference. */
    const struct device *display_dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_display));
    if (display_dev && device_is_ready(display_dev)) {
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
		lv_obj_set_style_bg_image_src(screen, status_screen_current_background(), LV_PART_MAIN);
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

    if (ARRAY_SIZE(status_backgrounds) > 1) {
        k_timer_init(&status_background_timer, status_background_timer_handler, NULL);
        k_timer_start(&status_background_timer,
                      K_MINUTES(CONFIG_XIAORD_BG_ROTATE_INTERVAL_MIN),
                      K_MINUTES(CONFIG_XIAORD_BG_ROTATE_INTERVAL_MIN));
    }

	k_timer_init(&status_screen_idle_timer, status_screen_idle_timeout, NULL);
	k_timer_start(&status_screen_idle_timer, K_MSEC(STATUS_SCREEN_IDLE_TIMEOUT_MS), K_NO_WAIT);

	return s_pages[0].screen;
}
