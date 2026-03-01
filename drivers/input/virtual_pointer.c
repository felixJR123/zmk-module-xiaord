/*
 * SPDX-License-Identifier: MIT
 *
 * Virtual pointer driver (zmk,virtual-pointer).
 *
 * Receives touch frames via zmk_virtual_pointer_feed() from the display layer
 * (status_screen.c reads LVGL indev events and calls this API).
 * Emits REL_X/REL_Y + BTN_0/BTN_1 as its own virtual input device for ZMK's
 * input-listener pipeline.
 *
 * Features:
 *   - Frame-delta ABS→REL conversion with configurable sensitivity
 *   - Tap gesture → BTN_0 (left click)
 *   - Long-press gesture → BTN_1 (right click)
 *   - Swipe detection (cancels tap/long-press, enables inertial cursor)
 *   - Inertial cursor after flick
 */

#define DT_DRV_COMPAT zmk_virtual_pointer

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <math.h>
#include <stdlib.h>

#include "virtual_pointer.h"

#define SENSITIVITY        ((float)CONFIG_ZMK_VIRTUAL_POINTER_SENSITIVITY_X10 / 10.0f)
#define MIN_BTN_INTERVAL   CONFIG_ZMK_VIRTUAL_POINTER_MIN_BTN_INTERVAL
#define LONG_PRESS_TIME    CONFIG_ZMK_VIRTUAL_POINTER_LONG_PRESS_TIME
#define SWIPE_THRESHOLD    CONFIG_ZMK_VIRTUAL_POINTER_SWIPE_THRESHOLD

#define INERTIA_INTERVAL   CONFIG_ZMK_VIRTUAL_POINTER_INERTIA_INTERVAL
#define VEL_THRESHOLD      ((float)CONFIG_ZMK_VIRTUAL_POINTER_INERTIA_THRESHOLD_X100 / 100.0f)
#define VEL_DECAY          ((float)CONFIG_ZMK_VIRTUAL_POINTER_INERTIA_DECAY_X100 / 100.0f)

LOG_MODULE_REGISTER(virtual_pointer, CONFIG_INPUT_LOG_LEVEL);

enum gesture_state { ST_IDLE, ST_TOUCH };
static const char *state_names[] = { "IDLE", "TOUCH" };
enum gesture_event { EV_DOWN, EV_UP, EV_TIMEOUT };

struct btn_task {
	uint16_t code;
	int value;
	uint32_t timestamp;
};

struct vpointer_data {
	const struct device *dev;
	struct k_work_delayable task_processor;
	struct k_work_delayable eval_timer;
	struct k_work_delayable inertial_work;
	struct k_msgq task_msgq;
	struct btn_task task_buf[8];
	enum gesture_state state;
	bool has_moved;
	int16_t start_x, start_y;   /* ABS coords at touch-down */
	int16_t last_x, last_y;     /* ABS coords from previous frame */
	bool has_last;               /* whether last_* is valid */
	uint32_t last_sample_time, delta_time;
	float v_delta_x, v_delta_y; /* inertia velocity */
};

/* ── Task queue: deferred button press/release ─────────────────────────── */

static void push_task(struct vpointer_data *data, uint16_t code, int value, uint32_t delay)
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
	struct vpointer_data *data = CONTAINER_OF(dwork, struct vpointer_data, task_processor);
	struct btn_task task;
	uint32_t now = k_uptime_get_32();

	while (k_msgq_peek(&data->task_msgq, &task) == 0) {
		if (now >= task.timestamp) {
			k_msgq_get(&data->task_msgq, &task, K_NO_WAIT);
			input_report_key(data->dev, task.code, task.value, true, K_FOREVER);
			LOG_INF("Exec Task: BTN %d=%d", task.code, task.value);
		} else {
			k_work_reschedule(&data->task_processor, K_MSEC(task.timestamp - now));
			break;
		}
	}
}

/* ── Inertial cursor ────────────────────────────────────────────────────── */

static void vpointer_inertial_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct vpointer_data *data = CONTAINER_OF(dwork, struct vpointer_data, inertial_work);

	data->v_delta_x *= VEL_DECAY;
	data->v_delta_y *= VEL_DECAY;

	if (fabsf(data->v_delta_x) >= 1.0f || fabsf(data->v_delta_y) >= 1.0f) {
		input_report_rel(data->dev, INPUT_REL_X, (int16_t)data->v_delta_x, false, K_FOREVER);
		input_report_rel(data->dev, INPUT_REL_Y, (int16_t)data->v_delta_y, true, K_FOREVER);
		k_work_reschedule(&data->inertial_work, K_MSEC(INERTIA_INTERVAL));
	}
}

/* ── Gesture state machine ──────────────────────────────────────────────── */

static void handle_gesture(struct vpointer_data *data, enum gesture_event event, bool has_moved)
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
			LOG_WRN("Tap canceled: Move detected");
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
	struct vpointer_data *data = CONTAINER_OF(dwork, struct vpointer_data, eval_timer);
	handle_gesture(data, EV_TIMEOUT, data->has_moved);
}

/* ── Movement helpers ───────────────────────────────────────────────────── */

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

/* ── Public API ─────────────────────────────────────────────────────────── */

void zmk_virtual_pointer_feed(const struct device *dev, int16_t x, int16_t y, bool pressed)
{
	struct vpointer_data *data = dev->data;
	uint32_t now = k_uptime_get_32();

	if (pressed) {
		if (!data->has_last) {
			/* Touch-down: initialize baseline */
			if (IS_ENABLED(CONFIG_ZMK_VIRTUAL_POINTER_INERTIA)) {
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
			/* Dragging: check swipe threshold and emit REL movement */
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
		/* Touch-up */
		handle_gesture(data, EV_UP, data->has_moved);

		if (IS_ENABLED(CONFIG_ZMK_VIRTUAL_POINTER_INERTIA) &&
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

/* ── Driver initialization ──────────────────────────────────────────────── */

static int vpointer_init(const struct device *dev)
{
	struct vpointer_data *data = dev->data;

	data->dev = dev;
	data->state = ST_IDLE;
	k_work_init_delayable(&data->task_processor, task_processor_handler);
	k_work_init_delayable(&data->eval_timer, eval_timer_handler);
	k_work_init_delayable(&data->inertial_work, vpointer_inertial_handler);
	k_msgq_init(&data->task_msgq, (char *)data->task_buf, sizeof(struct btn_task), 8);

	LOG_INF("virtual_pointer initialized");
	return 0;
}

/* ── Multi-instance macro ───────────────────────────────────────────────── */

#define VPOINTER_DEFINE(inst)                                                   \
	static struct vpointer_data vpointer_data_##inst;                       \
	DEVICE_DT_INST_DEFINE(inst, vpointer_init, NULL,                        \
			      &vpointer_data_##inst, NULL,                      \
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(VPOINTER_DEFINE)
