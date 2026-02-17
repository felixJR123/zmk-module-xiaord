
#define DT_DRV_COMPAT chipsemi_chsc6x_custom

#include <zephyr/sys/byteorder.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

struct chsc6x_custom_config {
	struct i2c_dt_spec i2c;
	const struct gpio_dt_spec int_gpio;
};

struct chsc6x_custom_data {
	const struct device *dev;
	struct k_work work;
	struct k_work_delayable inertial_work; // 慣性用タイマーワーク
	struct gpio_callback int_gpio_cb;

	uint8_t last_col;
	uint8_t last_row;
	bool last_pressed;

	/* 慣性計算用 */
	float delta_x;
	float delta_y;
	uint32_t last_time;
	uint32_t delta_time;
};

#define CHSC6X_READ_ADDR             0
#define CHSC6X_READ_LENGTH           5
#define CHSC6X_OUTPUT_POINTS_PRESSED 0
#define CHSC6X_OUTPUT_COL            2
#define CHSC6X_OUTPUT_ROW            4

/* 慣性パラメータ (調整可能) */
#define INERTIA_INTERVAL_MS    20    // 慣性移動の更新周期 (ANIMATE_MSEC相当)
#define VELOCITY_THRESHOLD     0.5f  // 慣性を開始する最小速度
#define VELOCITY_DECAY         0.70f // 減衰率 (decay_percent相当)

LOG_MODULE_REGISTER(chsc6x_custom, CONFIG_INPUT_LOG_LEVEL);

/* --- 慣性ワークハンドラ --- */
static void chsc6x_inertial_handler(struct k_work *work)
{
	struct k_work_delayable *d_work = k_work_delayable_from_work(work);
	struct chsc6x_custom_data *data = CONTAINER_OF(d_work, struct chsc6x_custom_data, inertial_work);

	/* 速度を減衰させる */
	data->delta_x *= VELOCITY_DECAY;
	data->delta_y *= VELOCITY_DECAY;

	/* 速度が十分に残っていれば移動を報告して再スケジューリング */
	if (fabsf(data->delta_x) >= 1.0f || fabsf(data->delta_y) >= 1.0f) {
		input_report_rel(data->dev, INPUT_REL_X, (int16_t)data->delta_x, false, K_FOREVER);
		input_report_rel(data->dev, INPUT_REL_Y, (int16_t)data->delta_y, true, K_FOREVER);
		k_work_reschedule(&data->inertial_work, K_MSEC(INERTIA_INTERVAL_MS));
	}
}

static int chsc6x_custom_process(const struct device *dev)
{
	uint8_t output[CHSC6X_READ_LENGTH];
	uint8_t row, col;
	bool is_pressed;
	int ret;
	uint32_t now = k_uptime_get_32();

	const struct chsc6x_custom_config *cfg = dev->config;
	struct chsc6x_custom_data *data = dev->data;

	ret = i2c_burst_read_dt(&cfg->i2c, CHSC6X_READ_ADDR, output, CHSC6X_READ_LENGTH);
	if (ret < 0) {
		LOG_ERR("Could not read data: %i", ret);
		return -ENODATA;
	}

	is_pressed = output[CHSC6X_OUTPUT_POINTS_PRESSED];
	col = output[CHSC6X_OUTPUT_COL];
	row = output[CHSC6X_OUTPUT_ROW];

	LOG_DBG("Raw data: pressed=%d, col=%d, row=%d", is_pressed, col, row);

	if (is_pressed) {
		if (!data->last_pressed) {
			/* --- タッチ開始 --- */
			k_work_cancel_delayable(&data->inertial_work); // 実行中の慣性を止める
			data->delta_x = 0;
			data->delta_y = 0;
		} else {
			/* --- 移動中 --- */
			data->delta_x = (float)((int16_t)col - (int16_t)data->last_col);
			data->delta_y = (float)((int16_t)row - (int16_t)data->last_row);
			data->delta_time = now - data->last_time;

			if (data->delta_x != 0 || data->delta_y != 0) {
				input_report_rel(dev, INPUT_REL_X, (int16_t)data->delta_x, false, K_FOREVER);
				input_report_rel(dev, INPUT_REL_Y, (int16_t)data->delta_y, true, K_FOREVER);
			}
		}
		data->last_col = col;
		data->last_row = row;
		data->last_time = now;
	} else {
		if (data->last_pressed) {
			/* --- タッチ終了 --- */
			if (data->delta_time > 0) {
				/* 離した瞬間の速度を計算 */
				float velocity = sqrtf(data->delta_x * data->delta_x + 
									   data->delta_y * data->delta_y) / (float)data->delta_time;

				/* 速度がしきい値を超えていれば慣性を開始 */
				if (velocity > VELOCITY_THRESHOLD) {
					k_work_reschedule(&data->inertial_work, K_MSEC(INERTIA_INTERVAL_MS));
				}
			}
		}
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

static int chsc6x_custom_chip_init(const struct device *dev)
{
	const struct chsc6x_custom_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	return 0;
}

static int chsc6x_custom_init(const struct device *dev)
{
	struct chsc6x_custom_data *data = dev->data;
	int ret;

	LOG_INF("Initializing CHSC6X custom driver");

	data->dev = dev;
	data->last_col = 0;
	data->last_row = 0;
	data->last_pressed = false;

	k_work_init(&data->work, chsc6x_custom_work_handler);

	const struct chsc6x_custom_config *config = dev->config;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO port %s not ready", config->int_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO interrupt: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, chsc6x_custom_isr_handler, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback: %d", ret);
		return ret;
	}

	ret = chsc6x_custom_chip_init(dev);
	if (ret < 0) {
		return ret;
	}

	k_work_init(&data->work, chsc6x_custom_work_handler);
	k_work_init_delayable(&data->inertial_work, chsc6x_inertial_handler);

	LOG_INF("CHSC6X custom driver initialized successfully");
	return 0;
};

#define CHSC6X_CUSTOM_DEFINE(index)                                                                       \
	static const struct chsc6x_custom_config chsc6x_custom_config_##index = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.int_gpio = GPIO_DT_SPEC_INST_GET(index, irq_gpios),                               \
	};                                                                                         \
	static struct chsc6x_custom_data chsc6x_custom_data_##index;                                             \
	DEVICE_DT_INST_DEFINE(index, chsc6x_custom_init, NULL, &chsc6x_custom_data_##index,                      \
			      &chsc6x_custom_config_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,     \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(CHSC6X_CUSTOM_DEFINE)