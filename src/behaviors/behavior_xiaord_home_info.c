// SPDX-License-Identifier: MIT
//
// ZMK behavior that requests a home screen info visibility toggle from the
// LVGL display thread.

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#define DT_DRV_COMPAT zmk_behavior_xiaord_home_info

extern void xiaord_home_info_request_toggle(void);

static int on_binding_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event)
{
    xiaord_home_info_request_toggle();
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_binding_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event)
{
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_xiaord_home_info_driver_api = {
    .binding_pressed  = on_binding_pressed,
    .binding_released = on_binding_released,
};

static int behavior_xiaord_home_info_init(const struct device *dev)
{
    ARG_UNUSED(dev);
    return 0;
}

#define XIAORD_HOME_INFO_INST(n) \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_xiaord_home_info_init, NULL, NULL, NULL, \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                            &behavior_xiaord_home_info_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XIAORD_HOME_INFO_INST)
