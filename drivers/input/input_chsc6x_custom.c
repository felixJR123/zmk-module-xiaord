#define DT_DRV_COMPAT chipsemi_chsc6x_custom

#include <zephyr/sys/byteorder.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <math.h>
#include <stdlib.h>

/* --- 定数定義 --- */
#define CHSC6X_READ_ADDR             0
#define CHSC6X_READ_LENGTH           5
#define CHSC6X_OUTPUT_POINTS_PRESSED 0
#define CHSC6X_OUTPUT_COL            2
#define CHSC6X_OUTPUT_ROW            4

/* --- パラメータ再設定 --- */
/* --- 調整パラメータ --- */
#define SENSITIVITY            2.0f  // 移動感度
#define MIN_BTN_INTERVAL_MS    20    // ボタンの最小パルス幅 (ms)
#define LONG_PRESS_TIME_MS     600   // 右クリック判定時間
#define SWIPE_THRESHOLD        10    // 移動と判定する閾値 (px)

/* --- 慣性パラメータ --- */
#define INERTIA_INTERVAL_MS    20    
#define VELOCITY_THRESHOLD     0.8f  
#define VELOCITY_DECAY         0.95f 

LOG_MODULE_REGISTER(chsc6x_custom, CONFIG_INPUT_LOG_LEVEL);

/* --- 型定義とログ用文字列 --- */
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
    struct k_work_delayable inertial_work;
    struct k_msgq task_msgq;
    struct btn_task task_buf[8];
    enum gesture_state state;
    bool has_moved;
    uint8_t start_col, start_row, last_col, last_row;
    bool last_pressed;
    float v_delta_x, v_delta_y;
    uint32_t last_sample_time, delta_time;
    struct gpio_callback int_gpio_cb;
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

    // 減衰計算
    data->v_delta_x *= VELOCITY_DECAY;
    data->v_delta_y *= VELOCITY_DECAY;

    // 一定速度以下になるまで移動レポートを継続
    if (fabsf(data->v_delta_x) >= 1.0f || fabsf(data->v_delta_y) >= 1.0f) {
        input_report_rel(data->dev, INPUT_REL_X, (int16_t)data->v_delta_x, false, K_FOREVER);
        input_report_rel(data->dev, INPUT_REL_Y, (int16_t)data->v_delta_y, true, K_FOREVER);
        k_work_reschedule(&data->inertial_work, K_MSEC(INERTIA_INTERVAL_MS));
    }
}

static void handle_gesture(const struct device *dev, enum gesture_event event) {
    struct chsc6x_custom_data *data = dev->data;
    enum gesture_state old_state = data->state;

    switch (data->state) {
    case ST_IDLE:
        if (event == EV_DOWN) {
            data->has_moved = false;
            data->state = ST_TOUCH;
            k_work_reschedule(&data->eval_timer, K_MSEC(LONG_PRESS_TIME_MS));
        }
        break;

    case ST_TOUCH:
        if (data->has_moved) {
            /* 移動検知によりクリックをキャンセル */
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

    /* 慣性開始判定 */
    if (event == EV_UP && data->has_moved && data->delta_time > 0) {
        float v = sqrtf(data->v_delta_x * data->v_delta_x + data->v_delta_y * data->v_delta_y) / (float)data->delta_time;
        if (v > 0.8f) k_work_reschedule(&data->inertial_work, K_MSEC(20));
    }
}

static void eval_timer_handler(struct k_work *work)
{
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct chsc6x_custom_data *data = CONTAINER_OF(dwork, struct chsc6x_custom_data, eval_timer);
    handle_gesture(data->dev, EV_TIMEOUT);
}

static int chsc6x_custom_process(const struct device *dev) {
    struct chsc6x_custom_data *data = dev->data;
    const struct chsc6x_custom_config *cfg = dev->config;
    uint8_t out[5];
    uint32_t now = k_uptime_get_32();

    if (i2c_burst_read_dt(&cfg->i2c, 0, out, 5) < 0) return -EIO;

    bool pr = out[0]; uint8_t c = out[2], r = out[4];

    if (pr) {
        if (!data->last_pressed) {
            k_work_cancel_delayable(&data->inertial_work);
            data->start_col = c; data->start_row = r;
            data->v_delta_x = 0; data->v_delta_y = 0;
            handle_gesture(dev, EV_DOWN);
        } else {
            int dist = abs(c - data->start_col) + abs(r - data->start_row);
            if (dist > SWIPE_THRESHOLD) data->has_moved = true;

            float dx = (float)((int16_t)c - (int16_t)data->last_col) * SENSITIVITY;
            float dy = (float)((int16_t)r - (int16_t)data->last_row) * SENSITIVITY;
            data->delta_time = now - data->last_sample_time;

            if (dx != 0 || dy != 0) {
                input_report_rel(dev, INPUT_REL_X, (int16_t)dx, false, K_FOREVER);
                input_report_rel(dev, INPUT_REL_Y, (int16_t)dy, true, K_FOREVER);
                data->v_delta_x = dx;
                data->v_delta_y = dy;
            }
        }
        data->last_col = c; data->last_row = r;
        data->last_sample_time = now;
    } else if (data->last_pressed) {
        handle_gesture(dev, EV_UP);
    }
    data->last_pressed = pr;
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
    k_work_init_delayable(&data->inertial_work, chsc6x_inertial_handler);
    k_msgq_init(&data->task_msgq, (char *)data->task_buf, sizeof(struct btn_task), 8);

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