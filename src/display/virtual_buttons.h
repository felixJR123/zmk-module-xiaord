/*
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/sys/slist.h>

struct zmk_widget_virtual_buttons {
    sys_snode_t node;
    lv_obj_t *obj; /* button screen (lv_obj_create(NULL)) */
};

int zmk_widget_virtual_buttons_init(struct zmk_widget_virtual_buttons *widget,
                                    lv_obj_t *return_screen);
lv_obj_t *zmk_widget_virtual_buttons_obj(struct zmk_widget_virtual_buttons *widget);
