/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 * Copyright (c) 2023 Gavin Hurlbut
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC driver for the MCP3422/23/24 ADCs.
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(adc_mcp342x, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

enum {
	

enum {
	MCP342X_CONFIG_12_240 = 0,
	MCP342X_CONFIG_14_60  = 1,
	MCP342X_CONFIG_16_15  = 2,
	MCP342X_CONFIG_18_3   = 3,
}

struct mcp342x_config {
	struct i2c_dt_spec bus;
	uint8_t channels;
};

struct mcp342x_data {
	struct adc_context ctx;
	const struct device *dev;
	void *buffer;
	void *repeat_buffer;
	uint8_t channels;
	uint8_t differential;
	uint8_t configval;
	uint32_t ready_time;
	struct k_thread thread;
	struct k_sem sem;

	K_KERNEL_STACK_MEMBER(stack,
			CONFIG_ADC_MCP342X_ACQUISITION_THREAD_STACK_SIZE);
};

static int mcp342x_read_conversion(const struct device *dev, uint32_t *buf)
{
	const struct mcp342x_config *config = dev->config;
	uint8_t resolution = config->resolution;

	int ret;
	uint32_t reg_val = 0;
	uint8_t *data = (uint8_t *)&reg_val;

	uint8_t count = (resolution == 18 ? 3 : 2);

	ret = i2c_read_dt(&config->bus, data, count + 1);
	if (ret != 0) {
		LOG_ERR("MCP342X[0x%X]: error reading register 0x%X (%d)",
			config->bus.addr, reg_addr, ret);
		return ret;
	}

	if (resolution != 18) {
		reg_val >>= 8;
	}

	if (reg_val & 0x00000080) {
		return 0;
	}

	uint32_t mask = BIT_MASK(resolution);
	*buf = (reg_val >> 8) & mask;

	return 0;
}

static int mcp342x_write_config(const struct device *dev, uint8_t reg_val)
{
	const struct mcp342x_config *config = dev->config;
	int ret;

	ret = i2c_write_dt(&config->bus, &reg_val, 1);

	if (ret != 0) {
		LOG_ERR("MCP342X[0x%X]: error writing config register (%d)",
			config->bus.addr, ret);
	}

	return ret;
}

static int mcp342x_start_conversion(const struct device *dev)
{
	struct mcp342x_data *data = dev->data;
	uint8_t configval = data->configval;

	configval |= 0x10;
	return mcp342x_write_config(dev, configval);
}


static int mcp342x_get_sample_rate_bits(uint8_t resolution) {
	switch(resolution) {
		case 12:
			return MCP342X_CONFIG_12_240;	// 12 bits, 240 SPS
		case 14:
			return MCP342X_CONFIG_14_60;	// 14 bits, 60 SPS
		case 16:
			return MCP342X_CONFIG_16_15;	// 16 bits, 15 SPS
		case 18:
			return MCP342X_CONFIG_18_3;	// 18 biths, 3.75 SPS
		default:
			return -1;
	}
}

static uint32_t mcp342x_get_timer_delay_us(uint8_t resolution) {
	switch(resolution) {
		case 12:
			return (1000000 / 240);
		case 14:
			return (1000000 / 60);
		case 16:
			return (1000000 / 15);
		case 18:
			return (100000000 / 375);
		default:
			return -1;
	}
}


static int mcp342x_get_gain_bits(uint8_t gain) {
	switch(resolution) {
		case ADC_GAIN_1:
			return 0;
		case ADC_GAIN_2:
			return 1;
		case ADC_GAIN_4:
			return 2;
		case ADC_GAIN_8:
			return 3;
		default:
			return -1;
	}
}

static int mcp342x_wait_data_ready(const struct device *dev)
{
	int rc = 0;
	struct mcp342x_data *data = dev->data;

	k_sleep(data->ready_time);

#if 0
	uint16_t status = 0;

	rc = mcp342x_read_reg(dev, ADS1X1X_REG_CONFIG, &status);
	if (rc != 0) {
		return rc;
	}

	while (!(status & ADS1X1X_CONFIG_OS)) {
		k_sleep(K_USEC(100));
		rc = ads1x1x_read_reg(dev, ADS1X1X_REG_CONFIG, &status);
		if (rc != 0) {
			return rc;
		}
	}
#endif
	return rc;
}

static int mcp342x_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct mcp342x_config *config = dev->config;
	struct mcp342x_data *data = dev->data;

	uint16_t configval = 0x0000;

	int gain_bits = mcp342x_get_gain_bits(channel_cfg->gain)
	if (gain_bits < 0) {
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("unsupported channel reference '%d'",
			channel_cfg->reference);
		return -ENOTSUP;
	}

	int sps_bits = mcp342x_get_sample_rate_bits(channel_cfg->resolution);
	if (sps_bits < 0) {
		LOG_ERR("unsupported resolution '%d'", channel_cfg->resolution);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("unsupported acquisition_time '%d'",
			channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (channel_cfg->channel_id >= config->channels) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}
	int channel_bits = channel_cfg->channel_id - 1;

	WRITE_BIT(data->differential, channel_cfg->channel_id,
		  channel_cfg->differential);

	configval |= (channel_bits & 0x03) << 5;
	configval |= (sps_bits & 0x03) << 2;
	configval |= gain_bits & 0x03;

	data->configval = configval;

	uint32_t us_delay = mcp342x_get_timer_delay_us(channel_cfg->resolution);
	us_delay += 25;
	data->ready_time = K_USEC(us_delay);

	return mcp342x_write_config(dev, configval);;
}

static int mcp342x_validate_buffer_size(const struct device *dev,
					const struct adc_sequence *sequence)
{
	const struct mcp342x_config *config = dev->config;
	struct mcp342x_data *data = dev->data;
	uint8_t channels = 0;
	size_t needed;
	uint32_t mask;

	for (mask = BIT(config->channels - 1); mask != 0; mask >>= 1) {
		if (mask & sequence->channels) {
			channels++;
		}
	}

	uint8_t wordsize = (sequence->resolution == 18 ? 32 : 16);
	needed = channels * wordsize / 8;
	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int mcp342x_start_read(const struct device *dev,
			      const struct adc_sequence *sequence)
{
	const struct mcp342x_config *config = dev->config;
	struct mcp342x_data *data = dev->data;
	int err;

	if (sequence->resolution != 12 && sequence->resolution != 14 &&
	    sequence->resolution != 16 && sequence->resolution != 18) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if (find_msb_set(sequence->channels) > config->channels) {
		LOG_ERR("unsupported channels in mask: 0x%08x",
			sequence->channels);
		return -ENOTSUP;
	}

	err = mcp342x_validate_buffer_size(dev, sequence);
	if (err) {
		LOG_ERR("buffer size too small");
		return err;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int mcp342x_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct mcp342x_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async ? true : false, async);
	err = mcp342x_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

static int mcp342x_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	return mcp342x_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct mcp342x_data *data = CONTAINER_OF(ctx, struct mcp342x_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	mcp342x_start_conversion(data->dev);
	k_sem_give(&data->sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct mcp342x_data *data = CONTAINER_OF(ctx, struct mcp342x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int mcp342x_read_channel(const struct device *dev, uint8_t channel,
				uint32_t *result)
{
	const struct mcp342x_config *config = dev->config;
	struct mcp342x_data *data = dev->data;

	err = mcp342x_read_conversion(dev, result);
	if (err) {
		return err;
	}

	*result &= BIT_MASK(config->resolution);

	return 0;
}

static void mcp342x_acquisition_thread(struct mcp342x_data *data)
{
	uint32_t result = 0;
	uint8_t *result_buf = &result;
	uint8_t *data_buf;
	uint8_t channel;
	int err;

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		err = mcp342x_wait_data_ready(dev);
		if (err) {
			LOG_ERR("failed to get ready status (err %d)", err);
			adc_context_complete(&data->ctx, err);
			break;
		}

		while (data->channels) {
			channel = find_lsb_set(data->channels) - 1;

			LOG_DBG("reading channel %d", channel);

			err = mcp342x_read_channel(data->dev, channel, &result);
			if (err) {
				LOG_ERR("failed to read channel %d (err %d)",
					channel, err);
				adc_context_complete(&data->ctx, err);
				break;
			}

			LOG_DBG("read channel %d, result = %d", channel,
				result);
			uint8_t count = (data->resolution == 18 ? 4 : 2);
			data_buf = data->buffer;

			for (int i = 0; i < count; i++) {
				*(data_buf++) = *(result_buf++);
			}

			WRITE_BIT(data->channels, channel, 0);
		}

		adc_context_on_sampling_done(&data->ctx, data->dev);
	}
}

static int mcp342x_init(const struct device *dev)
{
	const struct mcp342x_config *config = dev->config;
	struct mcp342x_data *data = dev->data;

	data->dev = dev;

	k_sem_init(&data->sem, 0, 1);

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	k_thread_create(&data->thread, data->stack,
			CONFIG_ADC_MCP342X_ACQUISITION_THREAD_STACK_SIZE,
			(k_thread_entry_t)mcp342x_acquisition_thread,
			data, NULL, NULL,
			CONFIG_ADC_MCP342X_ACQUISITION_THREAD_PRIO,
			0, K_NO_WAIT);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api mcp342x_adc_api = {
	.channel_setup = mcp342x_channel_setup,
	.read = mcp342x_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcp342x_read_async,
#endif
};

#define INST_DT_MCP342X(inst, t) DT_INST(inst, microchip_mcp##t)

#define MCP342X_DEVICE(t, n, ch) \
	static struct mcp342x_data mcp##t##_data_##n = { \
		ADC_CONTEXT_INIT_TIMER(mcp##t##_data_##n, ctx), \
		ADC_CONTEXT_INIT_LOCK(mcp##t##_data_##n, ctx), \
		ADC_CONTEXT_INIT_SYNC(mcp##t##_data_##n, ctx), \
	}; \
	static const struct mcp342x_config mcp##t##_config_##n = { \
		.bus = I2C_DT_SPEC_GET(INST_DT_MCP342X(n, t)), \
		.channels = ch, \
	}; \
	DEVICE_DT_DEFINE(INST_DT_MCP342X(n, t), \
			 &mcp342x_init, NULL, \
			 &mcp##t##_data_##n, \
			 &mcp##t##_config_##n, POST_KERNEL, \
			 CONFIG_ADC_INIT_PRIORITY, \
			 &mcp342x_adc_api)

/*
 * MCP3422: 2 channels
 */
#define MCP3422_DEVICE(n) MCP342X_DEVICE(3422, n, 2)

/*
 * MCP3423: 2 channels
 */
#define MCP3423_DEVICE(n) MCP342X_DEVICE(3423, n, 2)

/*
 * MCP3424: 4 channels
 */
#define MCP3424_DEVICE(n) MCP342X_DEVICE(3424, n, 4)

#define CALL_WITH_ARG(arg, expr) expr(arg)

#define INST_DT_MCP342X_FOREACH(t, inst_expr)			\
	LISTIFY(DT_NUM_INST_STATUS_OKAY(microchip_mcp##t),	\
		CALL_WITH_ARG, (;), inst_expr)

INST_DT_MCP342X_FOREACH(3422, MCP3422_DEVICE);
INST_DT_MCP342X_FOREACH(3423, MCP3423_DEVICE);
INST_DT_MCP342X_FOREACH(3424, MCP3424_DEVICE);
