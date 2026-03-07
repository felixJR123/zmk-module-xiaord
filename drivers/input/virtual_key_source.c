/*
 * SPDX-License-Identifier: MIT
 *
 * Virtual key source driver (zmk,virtual-key-source).
 *
 * Provides a virtual input device that emits INPUT_KEY_0..4 events.
 * The display layer (status_screen.c) calls ss_send_key() → input_report_key()
 * directly on this device; the touchpad_listener's input-processor-behaviors
 * maps those codes to ZMK behaviors.
 */

#define DT_DRV_COMPAT zmk_virtual_key_source

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(virtual_key_source, CONFIG_INPUT_LOG_LEVEL);

/* ── Driver data ─────────────────────────────────────────────────────────── */

struct vkey_data {
	const struct device *dev;
};

/* ── Driver initialization ──────────────────────────────────────────────── */

static int vkey_init(const struct device *dev)
{
	struct vkey_data *data = dev->data;

	data->dev = dev;
	LOG_INF("virtual_key_source initialized");
	return 0;
}

/* ── Multi-instance macro ───────────────────────────────────────────────── */

#define VKEY_DEFINE(inst)                                                       \
	static struct vkey_data vkey_data_##inst;                               \
	DEVICE_DT_INST_DEFINE(inst, vkey_init, NULL,                            \
			      &vkey_data_##inst, NULL,                          \
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(VKEY_DEFINE)
