#define DT_DRV_COMPAT chipsemi_chsc6x_custom

#include <zephyr/sys/byteorder.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <math.h>
#include <stdlib.h>

#define CHSC6X_READ_ADDR             0
#define CHSC6X_READ_LENGTH           5
#define CHSC6X_OUTPUT_POINTS_PRESSED 0
#define CHSC6X_OUTPUT_COL            2
#define CHSC6X_OUTPUT_ROW            4

#define SENSITIVITY            ((float)CONFIG_INPUT_CHSC6X_CUSTOM_SENSITIVITY_X10 / 10.0f)
#define MIN_BTN_INTERVAL_MS    CONFIG_INPUT_CHSC6X_CUSTOM_MIN_BTN_INTERVAL
#define LONG_PRESS_TIME_MS     CONFIG_INPUT_CHSC6X_CUSTOM_LONG_PRESS_TIME
#define SWIPE_THRESHOLD        CONFIG_INPUT_CHSC6X_CUSTOM_SWIPE_THRESHOLD

#define INERTIA_INTERVAL_MS    CONFIG_INPUT_CHSC6X_CUSTOM_INERTIA_INTERVAL
#define VELOCITY_THRESHOLD     ((float)CONFIG_INPUT_CHSC6X_CUSTOM_INERTIA_THRESHOLD_X100 / 100.0f)
#define VELOCITY_DECAY         ((float)CONFIG_INPUT_CHSC6X_CUSTOM_INERTIA_DECAY_X100 / 100.0f)

LOG_MODULE_REGISTER(chsc6x_custom, CONFIG_INPUT_LOG_LEVEL);

enum gesture_state { ST_IDLE, ST_TOUCH };
static const char *state_names[] = { "IDLE", "TOUCH" };
enum gesture_event { EV_DOWN, EV_UP, EV_TIMEOUT };
static const char *event_names[] = { "DOWN", "UP", "TIMEOUT" };

struct btn_task {
    uint16_t code;
    int value;
    uint32_t timestamp;
};

struct chsc6x_custom_config {
    struct i2c_dt_spec i2c;
    const struct gpio_dt_spec int_gpio;
};

struct chsc6x_custom_data {
    const struct device *dev;
    struct k_work work;
    struct k_work_delayable task_processor;
    struct k_work_delayable eval_timer;
    struct k_msgq task_msgq;
    struct btn_task task_buf[8];
    enum gesture_state state;
    bool has_moved;
    uint8_t start_col, start_row, last_col, last_row;
    bool last_pressed;
    uint32_t last_sample_time, delta_time;
    struct gpio_callback int_gpio_cb;
    struct k_work_delayable inertial_work;
    float v_delta_x, v_delta_y;
};

static void push_task(struct chsc6x_custom_data *data, uint16_t code, int value, uint32_t delay)
{
    struct btn_task task = { .code = code, .value = value, .timestamp = k_uptime_get_32() + delay };
    k_msgq_put(&data->task_msgq, &task, K_NO_WAIT);
    k_work_reschedule(&data->task_processor, K_NO_WAIT);
}

static void task_processor_handler(struct k_work *work)
{
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct chsc6x_custom_data *data = CONTAINER_OF(dwork, struct chsc6x_custom_data, task_processor);
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

static void chsc6x_inertial_handler(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct chsc6x_custom_data *data = CONTAINER_OF(dwork, struct chsc6x_custom_data, inertial_work);

    data->v_delta_x *= VELOCITY_DECAY;
    data->v_delta_y *= VELOCITY_DECAY;

    if (fabsf(data->v_delta_x) >= 1.0f || fabsf(data->v_delta_y) >= 1.0f) {
        input_report_rel(data->dev, INPUT_REL_X, (int16_t)data->v_delta_x, false, K_FOREVER);
        input_report_rel(data->dev, INPUT_REL_Y, (int16_t)data->v_delta_y, true, K_FOREVER);
        k_work_reschedule(&data->inertial_work, K_MSEC(INERTIA_INTERVAL_MS));
    }
}


static void handle_gesture(struct chsc6x_custom_data *data, enum gesture_event event, bool has_moved) {
    enum gesture_state old_state = data->state;

    switch (data->state) {
    case ST_IDLE:
        if (event == EV_DOWN) {
            data->state = ST_TOUCH;
            k_work_reschedule(&data->eval_timer, K_MSEC(LONG_PRESS_TIME_MS));
        }
        break;

    case ST_TOUCH:
        if (has_moved) {
            LOG_WRN("Tap canceled: Move detected");
            data->state = ST_IDLE;
            k_work_cancel_delayable(&data->eval_timer);
        } else if (event == EV_UP) {
            push_task(data, INPUT_BTN_0, 1, 0);
            push_task(data, INPUT_BTN_0, 0, MIN_BTN_INTERVAL_MS);
            data->state = ST_IDLE;
            k_work_cancel_delayable(&data->eval_timer);
        } else if (event == EV_TIMEOUT) {
            push_task(data, INPUT_BTN_1, 1, 0);
            push_task(data, INPUT_BTN_1, 0, MIN_BTN_INTERVAL_MS);
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
    struct chsc6x_custom_data *data = CONTAINER_OF(dwork, struct chsc6x_custom_data, eval_timer);
    handle_gesture(data, EV_TIMEOUT, data->has_moved);
}

static inline bool is_swipe_threshold_exceeded(const struct chsc6x_custom_data *data, uint8_t current_col, uint8_t current_row) {
    int manhattan_dist = abs(current_col - data->start_col) + abs(current_row - data->start_row);
    return manhattan_dist > SWIPE_THRESHOLD;
}

static inline float calculate_scaled_delta(uint8_t current_pos, uint8_t last_pos) {
    return (float)((int16_t)current_pos - (int16_t)last_pos) * SENSITIVITY;
}

static inline void report_relative_movement(const struct device *dev, float dx, float dy) {
    input_report_rel(dev, INPUT_REL_X, (int16_t)dx, false, K_FOREVER);
    input_report_rel(dev, INPUT_REL_Y, (int16_t)dy, true, K_FOREVER);
}

static inline void update_inertia_velocity(struct chsc6x_custom_data *data, float dx, float dy) {
    data->v_delta_x = dx;
    data->v_delta_y = dy;
}

static int chsc6x_custom_process(const struct device *dev) {
    struct chsc6x_custom_data *data = dev->data;
    const struct chsc6x_custom_config *cfg = dev->config;
    const bool inertial_cursor = IS_ENABLED(CONFIG_INPUT_CHSC6X_CUSTOM_INERTIA);
    uint8_t out[CHSC6X_READ_LENGTH];
    uint32_t now = k_uptime_get_32();

    if (i2c_burst_read_dt(&cfg->i2c, CHSC6X_READ_ADDR, out, CHSC6X_READ_LENGTH) < 0) return -EIO;

    bool is_pressed = out[CHSC6X_OUTPUT_POINTS_PRESSED];
    bool was_pressed = data->last_pressed;
    bool is_just_pressed  = !was_pressed && is_pressed;
    bool is_dragging      = was_pressed && is_pressed;
    bool is_just_released = was_pressed && !is_pressed;

    uint8_t col = out[CHSC6X_OUTPUT_COL];
    uint8_t row = out[CHSC6X_OUTPUT_ROW];


    if(is_just_pressed) {
        if (inertial_cursor) {
            k_work_cancel_delayable(&data->inertial_work);
            update_inertia_velocity(data, 0.0f, 0.0f);
        }
        data->start_col = col; 
        data->start_row = row;
        data->has_moved = false;
        handle_gesture(data, EV_DOWN, data->has_moved);

    } else if(is_dragging) {
        if (!data->has_moved && is_swipe_threshold_exceeded(data, col, row)) {
            data->has_moved = true;
        }
        float dx = calculate_scaled_delta(col, data->last_col);
        float dy = calculate_scaled_delta(row, data->last_row);
        data->delta_time = now - data->last_sample_time;
        if (dx != 0.0f || dy != 0.0f) {
            report_relative_movement(dev, dx, dy);
            update_inertia_velocity(data, dx, dy);
        }

    } else if(is_just_released){
        handle_gesture(data, EV_UP, data->has_moved);
        if (inertial_cursor) {
            if (data->has_moved && data->delta_time > 0) {
                float velocity = sqrtf(data->v_delta_x * data->v_delta_x + data->v_delta_y * data->v_delta_y) / (float)data->delta_time;
                if (velocity > VELOCITY_THRESHOLD) {
                    k_work_reschedule(&data->inertial_work, K_MSEC(INERTIA_INTERVAL_MS));
                }
            }
        }
    }

    if(is_just_pressed || is_dragging){
        input_report_abs(dev, INPUT_ABS_X, col, false, K_FOREVER);
        input_report_abs(dev, INPUT_ABS_Y, row, false, K_FOREVER);
        input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
    }else if (is_just_released){
        input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
    }

    if(is_pressed){
        data->last_col = col; 
        data->last_row = row;
        data->last_sample_time = now;
    }

    data->last_pressed = is_pressed;
    return 0;
}

static void chsc6x_custom_work_handler(struct k_work *work)
{
    struct chsc6x_custom_data *data = CONTAINER_OF(work, struct chsc6x_custom_data, work);
    chsc6x_custom_process(data->dev);
}

static void chsc6x_custom_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t mask)
{
    struct chsc6x_custom_data *data = CONTAINER_OF(cb, struct chsc6x_custom_data, int_gpio_cb);
    k_work_submit(&data->work);
}

static int chsc6x_custom_init(const struct device *dev)
{
    struct chsc6x_custom_data *data = dev->data;
    const struct chsc6x_custom_config *config = dev->config;
    int ret;

    LOG_INF("Init CHSC6X custom driver");
	data->dev = dev; 
    data->state = ST_IDLE;

    k_work_init(&data->work, chsc6x_custom_work_handler);
    k_work_init_delayable(&data->task_processor, task_processor_handler);
    k_work_init_delayable(&data->eval_timer, eval_timer_handler);
    k_msgq_init(&data->task_msgq, (char *)data->task_buf, sizeof(struct btn_task), 8);
    k_work_init_delayable(&data->inertial_work, chsc6x_inertial_handler);

    if (!i2c_is_ready_dt(&config->i2c)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }
    if (!gpio_is_ready_dt(&config->int_gpio)) {
        LOG_ERR("Interrupt GPIO not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("GPIO pin config failed: %d", ret);
        return ret;
    }
    ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("GPIO interrupt config failed: %d", ret);
        return ret;
    }
    gpio_init_callback(&data->int_gpio_cb, chsc6x_custom_isr_handler, BIT(config->int_gpio.pin));
    ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
    if (ret < 0) {
        LOG_ERR("GPIO callback add failed: %d", ret);
        return ret;
    }

    LOG_INF("Init CHSC6X custom driver OK");
    return 0;
}

#define CHSC6X_CUSTOM_DEFINE(index) \
    static const struct chsc6x_custom_config chsc6x_custom_config_##index = { \
        .i2c = I2C_DT_SPEC_INST_GET(index), \
        .int_gpio = GPIO_DT_SPEC_INST_GET(index, irq_gpios), \
    }; \
    static struct chsc6x_custom_data chsc6x_custom_data_##index; \
    DEVICE_DT_INST_DEFINE(index, chsc6x_custom_init, NULL, &chsc6x_custom_data_##index, \
                  &chsc6x_custom_config_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(CHSC6X_CUSTOM_DEFINE)