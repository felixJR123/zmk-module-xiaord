/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_code_canceller

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct cc_config {
    uint8_t type;
    size_t codes_size;
    uint16_t codes[];
};

static int cc_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                           uint32_t param2, struct zmk_input_processor_state *state) {
    const struct cc_config *cfg = dev->config;

    if (event->type != cfg->type) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    for (int i = 0; i < cfg->codes_size; i++) {
        if (cfg->codes[i] == event->code) {
            LOG_DBG("Cancelled event code %d", event->code);
            return ZMK_INPUT_PROC_STOP;
        }
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api cc_driver_api = {
    .handle_event = cc_handle_event,
};

#define CC_INST(n)                                                                                 \
    static const struct cc_config cc_config_##n = {                                                \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .codes_size = DT_INST_PROP_LEN(n, codes),                                                  \
        .codes = DT_INST_PROP(n, codes),                                                           \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &cc_config_##n, POST_KERNEL,                        \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &cc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CC_INST)
