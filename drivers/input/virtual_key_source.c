/*
 * SPDX-License-Identifier: MIT
 *
 * Virtual key source driver (zmk,virtual-key-source).
 *
 * Provides a virtual input device that emits INPUT_KEY_0..4 events.
 * The display layer (status_screen.c) calls ss_send_key() → input_report_key()
 * directly on this device; the touchpad_listener's input-processor-behaviors
 * maps those codes to ZMK behaviors.
 *
 * Cursor/gesture logic (ABS→REL conversion, tap, long-press, swipe, inertia)
 * is preserved below in #if 0 guards for future reference.
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

/* ── Cursor / gesture logic (preserved, currently unused) ─────────────── */

#if 0

#include <math.h>
#include <stdlib.h>

#define SENSITIVITY        ((float)CONFIG_ZMK_VIRTUAL_KEY_SOURCE_SENSITIVITY_X10 / 10.0f)
#define MIN_BTN_INTERVAL   CONFIG_ZMK_VIRTUAL_KEY_SOURCE_MIN_BTN_INTERVAL
#define LONG_PRESS_TIME    CONFIG_ZMK_VIRTUAL_KEY_SOURCE_LONG_PRESS_TIME
#define SWIPE_THRESHOLD    CONFIG_ZMK_VIRTUAL_KEY_SOURCE_SWIPE_THRESHOLD

#define INERTIA_INTERVAL   CONFIG_ZMK_VIRTUAL_KEY_SOURCE_INERTIA_INTERVAL
#define VEL_THRESHOLD      ((float)CONFIG_ZMK_VIRTUAL_KEY_SOURCE_INERTIA_THRESHOLD_X100 / 100.0f)
#define VEL_DECAY          ((float)CONFIG_ZMK_VIRTUAL_KEY_SOURCE_INERTIA_DECAY_X100 / 100.0f)

enum gesture_state { ST_IDLE, ST_TOUCH };
static const char *state_names[] = { "IDLE", "TOUCH" };
enum gesture_event { EV_DOWN, EV_UP, EV_TIMEOUT };

struct btn_task {
	uint16_t code;
	int value;
	uint32_t timestamp;
};

struct vkey_data_full {
	const struct device *dev;
	struct k_work_delayable task_processor;
	struct k_work_delayable eval_timer;
	struct k_work_delayable inertial_work;
	struct k_msgq task_msgq;
	struct btn_task task_buf[8];
	enum gesture_state state;
	bool has_moved;
	int16_t start_x, start_y;
	int16_t last_x, last_y;
	bool has_last;
	uint32_t last_sample_time, delta_time;
	float v_delta_x, v_delta_y;
};

static void push_task(struct vkey_data_full *data, uint16_t code, int value, uint32_t delay)
{
	struct btn_task task = {
		.code = code,
		.value = value,
		.timestamp = k_uptime_get_32() + delay,
	};
	k_msgq_put(&data->task_msgq, &task, K_NO_WAIT);
	k_work_reschedule(&data->task_processor, K_NO_WAIT);
}

static void task_processor_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct vkey_data_full *data = CONTAINER_OF(dwork, struct vkey_data_full, task_processor);
	struct btn_task task;
	uint32_t now = k_uptime_get_32();

	while (k_msgq_peek(&data->task_msgq, &task) == 0) {
		if (now >= task.timestamp) {
			k_msgq_get(&data->task_msgq, &task, K_NO_WAIT);
			input_report_key(data->dev, task.code, task.value, true, K_FOREVER);
		} else {
			k_work_reschedule(&data->task_processor, K_MSEC(task.timestamp - now));
			break;
		}
	}
}

static void vkey_inertial_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct vkey_data_full *data = CONTAINER_OF(dwork, struct vkey_data_full, inertial_work);

	data->v_delta_x *= VEL_DECAY;
	data->v_delta_y *= VEL_DECAY;

	if (fabsf(data->v_delta_x) >= 1.0f || fabsf(data->v_delta_y) >= 1.0f) {
		input_report_rel(data->dev, INPUT_REL_X, (int16_t)data->v_delta_x, false, K_FOREVER);
		input_report_rel(data->dev, INPUT_REL_Y, (int16_t)data->v_delta_y, true, K_FOREVER);
		k_work_reschedule(&data->inertial_work, K_MSEC(INERTIA_INTERVAL));
	}
}

static void handle_gesture(struct vkey_data_full *data, enum gesture_event event, bool has_moved)
{
	enum gesture_state old_state = data->state;

	switch (data->state) {
	case ST_IDLE:
		if (event == EV_DOWN) {
			data->state = ST_TOUCH;
			k_work_reschedule(&data->eval_timer, K_MSEC(LONG_PRESS_TIME));
		}
		break;

	case ST_TOUCH:
		if (has_moved) {
			data->state = ST_IDLE;
			k_work_cancel_delayable(&data->eval_timer);
		} else if (event == EV_UP) {
			push_task(data, INPUT_BTN_0, 1, 0);
			push_task(data, INPUT_BTN_0, 0, MIN_BTN_INTERVAL);
			data->state = ST_IDLE;
			k_work_cancel_delayable(&data->eval_timer);
		} else if (event == EV_TIMEOUT) {
			push_task(data, INPUT_BTN_1, 1, 0);
			push_task(data, INPUT_BTN_1, 0, MIN_BTN_INTERVAL);
			data->state = ST_IDLE;
		}
		break;
	}

	if (old_state != data->state) {
		LOG_INF("%s -> %s", state_names[old_state], state_names[data->state]);
	}
}

static void eval_timer_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct vkey_data_full *data = CONTAINER_OF(dwork, struct vkey_data_full, eval_timer);
	handle_gesture(data, EV_TIMEOUT, data->has_moved);
}

static inline bool is_swipe_threshold_exceeded(int16_t x, int16_t y,
						int16_t start_x, int16_t start_y)
{
	int manhattan = abs(x - start_x) + abs(y - start_y);
	return manhattan > SWIPE_THRESHOLD;
}

static inline float calculate_scaled_delta(int16_t current, int16_t last)
{
	return (float)(current - last) * SENSITIVITY;
}

static inline void report_relative_movement(const struct device *dev, float dx, float dy)
{
	input_report_rel(dev, INPUT_REL_X, (int16_t)dx, false, K_FOREVER);
	input_report_rel(dev, INPUT_REL_Y, (int16_t)dy, true, K_FOREVER);
}

void zmk_virtual_pointer_feed(const struct device *dev, int16_t x, int16_t y, bool pressed)
{
	struct vkey_data_full *data = dev->data;
	uint32_t now = k_uptime_get_32();

	if (pressed) {
		if (!data->has_last) {
			if (IS_ENABLED(CONFIG_ZMK_VIRTUAL_KEY_SOURCE_INERTIA)) {
				k_work_cancel_delayable(&data->inertial_work);
				data->v_delta_x = 0.0f;
				data->v_delta_y = 0.0f;
			}
			data->start_x = x;
			data->start_y = y;
			data->has_moved = false;
			handle_gesture(data, EV_DOWN, false);
			data->last_x = x;
			data->last_y = y;
			data->last_sample_time = now;
			data->has_last = true;
		} else {
			if (!data->has_moved &&
			    is_swipe_threshold_exceeded(x, y, data->start_x, data->start_y)) {
				data->has_moved = true;
			}

			float dx = calculate_scaled_delta(x, data->last_x);
			float dy = calculate_scaled_delta(y, data->last_y);
			data->delta_time = now - data->last_sample_time;

			if (dx != 0.0f || dy != 0.0f) {
				report_relative_movement(dev, dx, dy);
				data->v_delta_x = dx;
				data->v_delta_y = dy;
			}
			data->last_x = x;
			data->last_y = y;
			data->last_sample_time = now;
		}
	} else {
		handle_gesture(data, EV_UP, data->has_moved);

		if (IS_ENABLED(CONFIG_ZMK_VIRTUAL_KEY_SOURCE_INERTIA) &&
		    data->has_moved && data->delta_time > 0) {
			float velocity = sqrtf(data->v_delta_x * data->v_delta_x +
					       data->v_delta_y * data->v_delta_y) /
					 (float)data->delta_time;
			if (velocity > VEL_THRESHOLD) {
				k_work_reschedule(&data->inertial_work, K_MSEC(INERTIA_INTERVAL));
			}
		}
		data->has_last = false;
	}
}

#endif /* cursor/gesture logic */
