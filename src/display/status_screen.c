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
#include <errno.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/display.h>

#if IS_ENABLED(CONFIG_XIAORD_BG_SD)
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <ff.h>
#endif

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

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#if IS_ENABLED(CONFIG_XIAORD_BG_4) && defined(XIAORD_LOCAL_BG4_AVAILABLE)
extern const lv_image_dsc_t img_bg_4;
#define STATUS_BACKGROUND_IMAGE (&img_bg_4)
#elif IS_ENABLED(CONFIG_XIAORD_BG_4)
extern const lv_image_dsc_t img_bg_1;
#define STATUS_BACKGROUND_IMAGE (&img_bg_1)
#elif IS_ENABLED(CONFIG_XIAORD_BG_1)
extern const lv_image_dsc_t img_bg_1;
#define STATUS_BACKGROUND_IMAGE (&img_bg_1)
#elif IS_ENABLED(CONFIG_XIAORD_BG_2)
extern const lv_image_dsc_t img_bg_2;
#define STATUS_BACKGROUND_IMAGE (&img_bg_2)
#elif IS_ENABLED(CONFIG_XIAORD_BG_3)
extern const lv_image_dsc_t img_bg_3;
#define STATUS_BACKGROUND_IMAGE (&img_bg_3)
#elif IS_ENABLED(CONFIG_XIAORD_BG_5) || IS_ENABLED(CONFIG_XIAORD_BG_6)
extern const lv_image_dsc_t img_bg_1;
#define STATUS_BACKGROUND_IMAGE (&img_bg_1)
#else
#error "At least one CONFIG_XIAORD_BG_* option must be enabled"
#endif

static const lv_image_dsc_t *status_screen_current_background(void)
{
    return STATUS_BACKGROUND_IMAGE;
}
static const struct device *status_display;
static const struct device *status_backlight;
static uint32_t status_backlight_led;
#define STATUS_BACKLIGHT_LABEL "DISPLAY_BACKLIGHT"

static struct k_timer status_screen_idle_timer;
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

static uint8_t s_active_page;

#if IS_ENABLED(CONFIG_XIAORD_BG_SD)
#define STATUS_BG_SIZE 240
#define STATUS_BG_RGB565_BYTES (STATUS_BG_SIZE * STATUS_BG_SIZE * 2)
#define STATUS_BG_SD_PATH_MAX 96

static FATFS s_sd_fatfs;

static struct fs_mount_t s_sd_mount = {
    .type = FS_FATFS,
    .mnt_point = CONFIG_XIAORD_BG_SD_MOUNT_POINT,
    .fs_data = &s_sd_fatfs,
    .storage_dev = (void *)CONFIG_XIAORD_BG_SD_VOLUME_NAME,
    .flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
};

static uint8_t s_sd_bg_buf[STATUS_BG_RGB565_BYTES];
static lv_image_dsc_t s_sd_bg_dsc = {
    .header.cf     = LV_COLOR_FORMAT_RGB565,
    .header.magic  = LV_IMAGE_HEADER_MAGIC,
    .header.w      = STATUS_BG_SIZE,
    .header.h      = STATUS_BG_SIZE,
    .header.stride = STATUS_BG_SIZE * 2,
    .data_size     = STATUS_BG_RGB565_BYTES,
    .data          = s_sd_bg_buf,
};
static lv_obj_t *s_sd_bg_objs[PAGE_COUNT];
static uint16_t s_sd_bg_ids[CONFIG_XIAORD_BG_SD_MAX_FILES];
static size_t s_sd_bg_count;
static size_t s_sd_bg_index;
static bool s_sd_bg_ready;
static lv_timer_t *s_sd_bg_rotate_timer;
static lv_timer_t *s_sd_bg_retry_timer;

static bool status_screen_sd_filename_id(const char *name, uint16_t *id)
{
    if (!((name[0] == 'b' || name[0] == 'B') && (name[1] == 'g' || name[1] == 'G'))) {
        return false;
    }

    uint32_t value = 0;
    const char *p = name + 2;
    if (*p < '0' || *p > '9') {
        return false;
    }

    while (*p >= '0' && *p <= '9') {
        value = (value * 10U) + (uint32_t)(*p - '0');
        if (value > CONFIG_XIAORD_BG_SD_MAX_FILES) {
            return false;
        }
        p++;
    }

    if (value == 0U ||
        !(p[0] == '.' &&
          (p[1] == 'r' || p[1] == 'R') &&
          (p[2] == 'g' || p[2] == 'G') &&
          (p[3] == 'b' || p[3] == 'B') &&
          p[4] == '5' &&
          p[5] == '6' &&
          p[6] == '5' &&
          p[7] == '\0')) {
        return false;
    }

    *id = (uint16_t)value;
    return true;
}

static void status_screen_sd_add_id(uint16_t id)
{
    if (s_sd_bg_count >= ARRAY_SIZE(s_sd_bg_ids)) {
        return;
    }

    size_t pos = s_sd_bg_count;
    while (pos > 0 && s_sd_bg_ids[pos - 1] > id) {
        s_sd_bg_ids[pos] = s_sd_bg_ids[pos - 1];
        pos--;
    }
    s_sd_bg_ids[pos] = id;
    s_sd_bg_count++;
}

static int status_screen_sd_path(char *path, size_t path_len, uint16_t id)
{
    int ret = snprintf(path, path_len, "%s%s/bg%03u.rgb565",
                       CONFIG_XIAORD_BG_SD_MOUNT_POINT,
                       CONFIG_XIAORD_BG_SD_DIR,
                       (unsigned int)id);
    return (ret > 0 && (size_t)ret < path_len) ? 0 : -ENAMETOOLONG;
}

static int status_screen_sd_draw_index(size_t index)
{
    if (index >= s_sd_bg_count) {
        return -EINVAL;
    }

    char path[STATUS_BG_SD_PATH_MAX];
    int err = status_screen_sd_path(path, sizeof(path), s_sd_bg_ids[index]);
    if (err) {
        return err;
    }

    struct fs_file_t file;
    fs_file_t_init(&file);
    err = fs_open(&file, path, FS_O_READ);
    if (err) {
        LOG_WRN("SD background open failed: %s (%d)", path, err);
        return err;
    }

    size_t total = STATUS_BG_RGB565_BYTES;
    size_t off = 0;
    while (off < total) {
        ssize_t got = fs_read(&file, s_sd_bg_buf + off, total - off);
        if (got <= 0) {
            break;
        }
        off += (size_t)got;
    }
    fs_close(&file);

    if (off < total) {
        LOG_WRN("SD background short read: %u/%u bytes", (unsigned)off, (unsigned)total);
        return -EIO;
    }

    s_sd_bg_index = index;

    for (size_t i = 0; i < ARRAY_SIZE(s_sd_bg_objs); i++) {
        if (s_sd_bg_objs[i]) {
            lv_image_cache_drop(&s_sd_bg_dsc);
            lv_obj_invalidate(s_sd_bg_objs[i]);
        }
    }
    return 0;
}

static void status_screen_sd_invalidate_active_page(void)
{
    if (s_active_page < PAGE_COUNT && s_pages[s_active_page].screen) {
        lv_obj_invalidate(s_pages[s_active_page].screen);
    }
}

static void status_screen_sd_create_bg_obj(lv_obj_t *screen, size_t page_idx)
{
    if (page_idx >= ARRAY_SIZE(s_sd_bg_objs) || s_sd_bg_objs[page_idx]) {
        return;
    }

    lv_obj_t *img = lv_image_create(screen);
    lv_obj_set_pos(img, 0, 0);
    lv_obj_set_size(img, STATUS_BG_SIZE, STATUS_BG_SIZE);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(img, &s_sd_bg_dsc);
    lv_obj_move_background(img);
    s_sd_bg_objs[page_idx] = img;
}

static void status_screen_sd_set_mode(bool sd_background)
{
    for (size_t i = 0; i < PAGE_COUNT; i++) {
        lv_obj_t *screen = s_pages[i].screen;
        if (!screen) {
            continue;
        }

        if (sd_background) {
            lv_obj_set_style_bg_image_src(screen, NULL, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
            status_screen_sd_create_bg_obj(screen, i);
        } else {
            lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_bg_image_src(screen, status_screen_current_background(), LV_PART_MAIN);
        }
        lv_obj_invalidate(screen);
    }
}

static int status_screen_sd_mount(void)
{
    int err = disk_access_init(CONFIG_XIAORD_BG_SD_VOLUME_NAME);
    if (err && err != -EALREADY && err != -EBUSY) {
        LOG_WRN("SD disk init failed for %s: %d", CONFIG_XIAORD_BG_SD_VOLUME_NAME, err);
    }

    err = fs_mount(&s_sd_mount);
    if (err == -EALREADY || err == -EBUSY) {
        return 0;
    }
    if (err) {
        LOG_WRN("SD background mount failed for %s at %s: %d",
                CONFIG_XIAORD_BG_SD_VOLUME_NAME,
                CONFIG_XIAORD_BG_SD_MOUNT_POINT,
                err);
    }
    return err;
}

static int status_screen_sd_scan(void)
{
    char dir_path[STATUS_BG_SD_PATH_MAX];
    int ret = snprintf(dir_path, sizeof(dir_path), "%s%s",
                       CONFIG_XIAORD_BG_SD_MOUNT_POINT,
                       CONFIG_XIAORD_BG_SD_DIR);
    if (ret <= 0 || (size_t)ret >= sizeof(dir_path)) {
        return -ENAMETOOLONG;
    }

    struct fs_dir_t dir;
    struct fs_dirent entry;
    fs_dir_t_init(&dir);

    int err = fs_opendir(&dir, dir_path);
    if (err) {
        LOG_WRN("SD background directory unavailable: %s (%d)", dir_path, err);
        return err;
    }

    s_sd_bg_count = 0;
    while (fs_readdir(&dir, &entry) == 0 && entry.name[0] != '\0') {
        uint16_t id;
        if (entry.type != FS_DIR_ENTRY_FILE || entry.size != STATUS_BG_RGB565_BYTES) {
            continue;
        }
        if (status_screen_sd_filename_id(entry.name, &id)) {
            status_screen_sd_add_id(id);
        }
    }

    fs_closedir(&dir);
    LOG_INF("Found %u SD background(s)", (unsigned int)s_sd_bg_count);
    return s_sd_bg_count > 0 ? 0 : -ENOENT;
}

static bool status_screen_sd_init(void)
{
    if (s_sd_bg_ready) {
        return true;
    }

    if (status_screen_sd_mount() != 0 || status_screen_sd_scan() != 0) {
        LOG_WRN("Using compiled fallback background");
        return false;
    }

    s_sd_bg_ready = true;
    return true;
}

static void status_screen_sd_rotate_cb(lv_timer_t *timer)
{
    ARG_UNUSED(timer);
    ss_background_next();
}

static void status_screen_sd_stop_timer(void)
{
    if (!s_sd_bg_rotate_timer) {
        return;
    }
    lv_timer_del(s_sd_bg_rotate_timer);
    s_sd_bg_rotate_timer = NULL;
}

static void status_screen_sd_start_timer(void)
{
    if (CONFIG_XIAORD_BG_SD_ROTATE_MS <= 0 || s_sd_bg_count < 2) {
        return;
    }
    status_screen_sd_stop_timer();
    s_sd_bg_rotate_timer = lv_timer_create(status_screen_sd_rotate_cb,
                                           CONFIG_XIAORD_BG_SD_ROTATE_MS,
                                           NULL);
}

static void status_screen_sd_retry_stop(void)
{
    if (!s_sd_bg_retry_timer) {
        return;
    }

    lv_timer_del(s_sd_bg_retry_timer);
    s_sd_bg_retry_timer = NULL;
}

static void status_screen_sd_retry_cb(lv_timer_t *timer)
{
    ARG_UNUSED(timer);

    if (!status_screen_sd_init()) {
        return;
    }

    LOG_INF("SD background became available after boot");
    status_screen_sd_retry_stop();
    (void)status_screen_sd_draw_index(0);
    status_screen_sd_set_mode(true);
    status_screen_sd_start_timer();
}

static void status_screen_sd_start_retry_timer(void)
{
    if (CONFIG_XIAORD_BG_SD_RETRY_MS <= 0 || s_sd_bg_ready || s_sd_bg_retry_timer) {
        return;
    }

    s_sd_bg_retry_timer = lv_timer_create(status_screen_sd_retry_cb,
                                          CONFIG_XIAORD_BG_SD_RETRY_MS,
                                          NULL);
}
#endif /* CONFIG_XIAORD_BG_SD */

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

void ss_background_autoplay_start(void)
{
#if IS_ENABLED(CONFIG_XIAORD_BG_SD)
    status_screen_sd_start_timer();
#endif
}

void ss_background_autoplay_stop(void)
{
#if IS_ENABLED(CONFIG_XIAORD_BG_SD)
    status_screen_sd_stop_timer();
#endif
}

bool ss_background_next(void)
{
#if IS_ENABLED(CONFIG_XIAORD_BG_SD)
    if (!s_sd_bg_ready || s_sd_bg_count < 2) {
        return false;
    }

    size_t next = (s_sd_bg_index + 1) % s_sd_bg_count;
    if (status_screen_sd_draw_index(next) != 0) {
        return false;
    }

    if (s_sd_bg_rotate_timer) {
        status_screen_sd_start_timer();
    }
    status_screen_sd_invalidate_active_page();
    return true;
#else
    return false;
#endif
}

bool ss_background_prev(void)
{
#if IS_ENABLED(CONFIG_XIAORD_BG_SD)
    if (!s_sd_bg_ready || s_sd_bg_count < 2) {
        return false;
    }

    size_t prev = s_sd_bg_index == 0 ? s_sd_bg_count - 1 : s_sd_bg_index - 1;
    if (status_screen_sd_draw_index(prev) != 0) {
        return false;
    }

    if (s_sd_bg_rotate_timer) {
        status_screen_sd_start_timer();
    }
    status_screen_sd_invalidate_active_page();
    return true;
#else
    return false;
#endif
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
#if IS_ENABLED(CONFIG_XIAORD_BG_SD)
    bool sd_bg_ready = status_screen_sd_init();
    if (sd_bg_ready) {
        (void)status_screen_sd_draw_index(0);
    }
#endif

	/* Create an independent screen for each page */
	for (size_t i = 0; i < PAGE_COUNT; i++) {
		lv_obj_t *screen = lv_obj_create(NULL);
		lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
#if IS_ENABLED(CONFIG_XIAORD_BG_SD)
        if (sd_bg_ready) {
            lv_obj_set_style_bg_image_src(screen, NULL, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
            status_screen_sd_create_bg_obj(screen, i);
        } else {
            lv_obj_set_style_bg_image_src(screen, status_screen_current_background(), LV_PART_MAIN);
        }
#else
		lv_obj_set_style_bg_image_src(screen, status_screen_current_background(), LV_PART_MAIN);
#endif
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
#if IS_ENABLED(CONFIG_XIAORD_BG_SD)
    status_screen_sd_start_timer();
    status_screen_sd_start_retry_timer();
#endif

	return s_pages[0].screen;
}
