// SPDX-License-Identifier: MIT
//
// ZMK behavior that requests a home screen menu toggle from the LVGL display
// thread. Uses an atomic flag polled by an lv_timer in status_screen.c so
// that LVGL APIs are always called from the display thread.

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#define DT_DRV_COMPAT zmk_behavior_xiaord_menu

// Implemented in status_screen.c — sets an atomic flag that the LVGL timer
// picks up in the display thread to call home_buttons_toggle_visible().
extern void xiaord_menu_request_toggle(void);

static int on_binding_pressed(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event)
{
    xiaord_menu_request_toggle();
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_binding_released(struct zmk_behavior_binding *binding,
                                struct zmk_behavior_binding_event event)
{
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_xiaord_menu_driver_api = {
    .binding_pressed  = on_binding_pressed,
    .binding_released = on_binding_released,
};

static int behavior_xiaord_menu_init(const struct device *dev)
{
    ARG_UNUSED(dev);
    return 0;
}

#define XIAORD_MENU_INST(n) \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_xiaord_menu_init, NULL, NULL, NULL, \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                            &behavior_xiaord_menu_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XIAORD_MENU_INST)
