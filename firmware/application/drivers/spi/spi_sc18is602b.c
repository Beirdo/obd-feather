/*
 * Copyright (c) 2021 Marc Reilly - Creative Product Design
 * Copyright (c) 2023 Gavin Hurlbut
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_sc18is602b

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sc18is602b);

#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include "spi_context.h"

#define SPI_BUF_SIZE	16

struct sc18is602b_data {
	const struct device *dev;
	struct spi_context ctx;
};

struct sc18is602b_config {
	struct i2c_dt_spec bus;
	// Hot supporting yet
	// struct gpio_dt_spec irq_gpio;  
	// Aso not supporting the built-in GPOPs at this time
};

static int sc18is602b_write_reg(const struct device *dev, uint8_t function, uint8_t *value)
{
	sc18is602b_config *config = dev->config;
	uint8_t buf[2];
	int count = 1;

	buf[0] = function;
	if (value) {
		count++;
		buf[1] = *value;
	}

	return i2c_write_dt(config->bus, buf, count);
}

static int sc18is602b_read_reg(const struct device *dev, uint8_t function, uint8_t *value)
{
	sc18is602b_config *config = dev->config;

	return i2c_write_read_dt(config->bus, &function, 1, value, 1);
}


static int sc18is602b_write_buffer(const struct device *dev, uint8_t *function, const struct spi_buf *tx_buf)
{
	sc18is602b_config *config = dev->config;
	static uint8_t int_buf[MAX_BUF_SIZE];
	uint8_t *buf = tx_buf->buf;
	int count = tx_buf->len;

	if (function) {
		buf = int_buf;
		count++;

		if (count > MAX_BUF_SIZE) {
			return -ENOTSUP;
		}

		buf[0] = (1 << config->channel_id) & 0x0F;
		memcpy(&buf[1], tx_buf->buf, tx_buf->len);
	}

	return i2c_write_dt(config->bus, buf, count);
}

static int sc18is602b_write_read_buffer(const struct device *dev, uint8_t *function, uint8_t *tx_buf, uint8_t *rx_buf)
{
	int status;

	status = sc18is602b_send_buffer(dev, function, tx_buf);
	if (status)
	{
		return status;
	}
	return sc18is602b_read_buffer(dev, rx_buf);
}

static int sc18is602b_read_buffer(const struct device *dev, uint8_t *rx_buf)
{
	sc18is602b_config *config = dev->config;
	return i2c_read_dt(config->bus, rx_buf->buf, rx_buf->len);
}


static int sc18is602b_configure(const struct sc18is602b_config *info,
			    struct sc18is602b_data *data,
			    const struct spi_config *config)
{
	const struct device *dev = data->dev;
	uint8_t config_val = 0x00;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if (config->operation & (SPI_LINES_DUAL | SPI_LINES_QUAD)) {
		LOG_ERR("Unsupported configuration");
		return -ENOTSUP;
	}

	const int bits = SPI_WORD_SIZE_GET(config->operation);

	if (bits != 8) {
		LOG_ERR("Word sizes != 8 bits not supported");
		return -ENOTSUP;
	}

	config_val |= (!(!(config->operation & SPI_MODE_CPOL))) << 3;
	config_val |= (!(!(config->operation & SPI_MODE_CPHA))) << 2;
	config_val |= (!(!(config->operation & SPI_TRANSFER_LSB))) << 5;

	uint8_t freq_bits;
	uint32_t freq = config->frequency;

	if (freq >= 1843000) {
		// Peg to 1.843MHz
		freq_bits = 0;
	} else if (freq >= 461000) {
		// Peg to 461kHz
		freq_bits = 1;
	} else if (freq >= 115000) {
		// Peg to 115kHz
		freq_bits = 2;
	} else {
		// Peg to 58kHz
		freq_bits = 3;
	}

	config_val |= freq_bits;

	sc18is602b_write_reg(dev, 0xF0, &config_val);

	data->ctx.config = config;

	return 0;
}

static int sc18is602b_clear_interrupt(const struct device *dev) {
	return sc18is602b_write_reg(dev, 0xF1, NULL);
}

static int sc18is602b_idle_mode(const struct device *dev) {
	return sc18is602b_write_reg(dev, 0xF2, NULL);
}

static int sc18is602b_transceive(const struct device *dev,
			      const struct spi_config *spi_cfg,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs)
{
	const struct sc18is602b_config *info = dev->config;
	struct sc18is602b_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	size_t tx_count = 0;
	size_t rx_count = 0;
	const struct spi_buf *tx = NULL;
	const struct spi_buf *rx = NULL;
	int rc;

	rc = sc18is602b_configure(info, data, spi_cfg);
	if (rc < 0) {
		return rc;
	}

	spi_context_lock(ctx, false, NULL, NULL, spi_cfg);
	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	if (tx_bufs) {
		tx = tx_bufs->buffers;
		tx_count = tx_bufs->count;
	}

	if (rx_bufs) {
		rx = rx_bufs->buffers;
		rx_count = rx_bufs->count;
	}

	while (tx_count != 0 && rx_count != 0) {
		if (tx->buf == NULL) {
			sc18is602b_read_buffer(dev, rx);
		} else if (rx->buf == NULL) {
			sc18is602b_write_buffer(dev, NULL, tx);
		} else {
			sc18is602b_write_read_buffer(dev, NULL, tx, rx);
		}

		tx++;
		tx_count--;
		rx++;
		rx_count--;
	}

	for (; tx_count != 0; tx_count--) {
		sc18is602b_write_buffer(dev, NULL, tx++);
	}

	for (; rx_count != 0; rx_count--) {
		sc18is602b_read_buffer(dev, rx++);
	}

	spi_context_complete(ctx, dev, 0);

	return 0;
}

#ifdef CONFIG_SPI_ASYNC
static int sc18is602b_transceive_async(const struct device *dev,
				    const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs,
				    struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif

int sc18is602b_release(const struct device *dev,
			  const struct spi_config *config)
{
	struct sc18is602b_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	spi_context_unlock_unconditionally(ctx);
	return 0;
}

static struct spi_driver_api sc18is602b_api = {
	.transceive = sc18is602b_transceive,
	.release = sc18is602b_release,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = sc18is602b_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
};

int sc18is602b_init(const struct device *dev)
{
	const struct sc18is602b_config *config = dev->config;
	struct sc18is602b_data *data = dev->data;

	data->dev = dev;
	return 0;
}

#define SPI_SC18IS602B_INIT(inst)					\
	static struct sc18is602b_config sc18is602b_config_##inst = {	\
		.bus = I2C_DT_SPEC_GET(DT_INST(inst, nxp_sc18is602b)),  \
	};								\
									\
	static struct sc18is602b_data sc18is602b_data_##inst = {	\
		SPI_CONTEXT_INIT_LOCK(sc18is602b_data_##id, ctx),	\
		SPI_CONTEXT_INIT_SYNC(sc18is602b_data_##inst, ctx)	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			    sc18is602b_init,				\
			    NULL,					\
			    &sc18is602b_data_##inst,			\
			    &sc18is602b_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SPI_INIT_PRIORITY,			\
			    &sc18is602b_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SC18IS602B_INIT)
