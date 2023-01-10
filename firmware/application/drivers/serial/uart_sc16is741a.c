/* SC16IS741A serial driver */

#define DT_DRV_COMPAT nxp_sc16is741a

/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2020-2022 Intel Corp.
 * Copyright (c) 2023 Gavin Hurlbut
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief SC16IS741A Serial Driver
 *
 * This is based on the driver for the Intel SC16IS741A UART Chip used on the
 * PC 386.  It uses the SCCs in asynchronous mode only.
 *
 * Before individual UART port can be used, uart_sc16is741a_port_init() has to
 * be called to setup the port.
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/types.h>

#include <zephyr/init.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>

#include <zephyr/drivers/serial/uart_sc16is741a.h>

/* register definitions */

#define REG_THR   0x00  /* Transmitter holding reg.              */
#define REG_RHR   0x00  /* Receiver holding reg.                 */
#define REG_DLL	  0x00  /* Divisor Latch Low (with LCR7=1)       */
#define REG_IER   0x01  /* Interrupt enable reg.                 */
#define REG_DLH	  0x01  /* Divisor Latch High (with LCR7=1)      */
#define REG_IIR   0x02  /* Interrupt ID reg.                     */
#define REG_FCR   0x02  /* FIFO control reg.                     */
#define REG_EFR	  0x02  /* Enhanced Featured Reg (with LCR=0xBF) */
#define REG_LCR   0x03  /* Line control reg.                     */
#define REG_MCR   0x04  /* Modem control reg.                    */
#define REG_XON1  0x04  /* XON1 reg. (with LCR=0xBF)             */
#define REG_LSR   0x05  /* Line status reg.                      */
#define REG_XON2  0x04  /* XON2 reg. (with LCR=0xBF)             */
#define REG_MSR   0x06  /* Modem status reg.                     */
#define REG_XOFF1 0x06  /* XOFF1 reg. (with LCR=0xBF)            */
#define REG_TCR   0x06  /* Transmit Control reg.                 */
#define REG_SPR   0x07  /* ???? reg.                             */
#define REG_TLR   0x07  /* Trigger Level reg.                    */
#define REG_XOFF2 0x06  /* XOFF2 reg. (with LCR=0xBF)            */
#define REG_TXLVL 0x08  /* TX FIFO Level reg.                    */
#define REG_RXLVL 0x09  /* RX FIFO Level reg.                    */
#define REG_RST   0x0E	/* UART Reset                            */
#define REG_EFCR  0x0F	/* Extra Features Control reg.           */

/* equates for interrupt enable register */

#define IER_RXRDY 0x01 /* receiver data ready */
#define IER_TBE   0x02 /* transmit bit enable */
#define IER_LSR   0x04 /* line status interrupts */
#define IER_MSR   0x08 /* modem status interrupts (reserved) */
#define IER_SLEEP 0x10 /* enable sleep mode (mod when EFR4 set) */
#define IER_XOFF  0x20 /* XOFF interrupt (mod when EFR4 set)*/
#define IER_RTS	  0x40 /* RTS interrupt (mod when EFR4 set)*/
#define IER_CTS	  0x80 /* CTS interrupt (mod when EFR4 set)*/

/* equates for interrupt identification register */

#define IIR_MSTAT 0x00 /* modem status interrupt  */
#define IIR_NIP   0x01 /* no interrupt pending    */
#define IIR_THRE  0x02 /* transmit holding register empty interrupt */
#define IIR_RBRF  0x04 /* receiver buffer register full interrupt */
#define IIR_LS    0x06 /* receiver line status interrupt */
#define IIR_CH    0x0C /* Character timeout */
#define IIR_XOFF  0x10 /* received XOFF signal/special character */
#define IIR_CTS	  0x20 /* CTS/RTS change of state from active (LOw) to inactive (HIGH) */
#define IIR_MASK  0x37 /* interrupt id bits mask  */
#define IIR_ID    0x36 /* interrupt ID mask without NIP */
#define IIR_FE    0xC0 /* FIFO mode enabled */

/* equates for FIFO control register */

#define FCR_FIFO    	 0x01 /* enable XMIT and RCVR FIFO */
#define FCR_RCVRCLR 	 0x02 /* clear RCVR FIFO */
#define FCR_XMITCLR 	 0x04 /* clear XMIT FIFO */

#define FCR_TX_TRIG_MASK 0x30	/* bit mask for TX trigger level */
#define FCR_RX_TRIG_MASK 0xC0	/* bit mask for RX trigger level */

enum {
	FCR_TX_TRIG_8  = 0x00,
	FCR_TX_TRIG_16 = 0x10,
	FCR_TX_TRIG_32 = 0x20,
	FCR_TX_TRIG_56 = 0x30,
};

enum {
	FCR_RX_TRIG_8  = 0x00,
	FCR_RX_TRIG_16 = 0x40,
	FCR_RX_TRIG_56 = 0x80,
	FCR_RX_TRIG_60 = 0xC0,
};

/*
 * Per PC16550D (Literature Number: SNLS378B):
 *
 * RXRDY, Mode 0: When in the 16450 Mode (FCR0 = 0) or in
 * the FIFO Mode (FCR0 = 1, FCR3 = 0) and there is at least 1
 * character in the RCVR FIFO or RCVR holding register, the
 * RXRDY pin (29) will be low active. Once it is activated the
 * RXRDY pin will go inactive when there are no more charac-
 * ters in the FIFO or holding register.
 *
 * TXRDY, Mode 0: In the 16450 Mode (FCR0 = 0) or in the
 * FIFO Mode (FCR0 = 1, FCR3 = 0) and there are no charac-
 * ters in the XMIT FIFO or XMIT holding register, the TXRDY
 * pin (24) will be low active. Once it is activated the TXRDY
 * pin will go inactive after the first character is loaded into the
 * XMIT FIFO or holding register.
 */
#define FCR_MODE0 0x00 /* set receiver in mode 0 */

#if 0
/* the SC16IS471A does not support MODE1 */
#define FCR_MODE1 0x08 /* set receiver in mode 1 */
#endif

/* constants for line control register */

#define LCR_CS5 0x00   /* 5 bits data size */
#define LCR_CS6 0x01   /* 6 bits data size */
#define LCR_CS7 0x02   /* 7 bits data size */
#define LCR_CS8 0x03   /* 8 bits data size */
#define LCR_2_STB 0x04 /* 2 stop bits (1.5 stop bits at 5 bit word size) */
#define LCR_1_STB 0x00 /* 1 stop bit */
#define LCR_PEN 0x08   /* parity enable */
#define LCR_PDIS 0x00  /* parity disable */
#define LCR_EPS 0x10   /* even parity select */
#define LCR_SP 0x20    /* stick parity select */
#define LCR_SBRK 0x40  /* break control bit */
#define LCR_DLAB 0x80  /* divisor latch access enable */

/* constants for the modem control register */

#if 0
#define MCR_DTR 	0x01 /* dtr output */
#endif
#define MCR_RTS 	0x02 /* rts output */
#define MCR_TLR_TCR 	0x04 /* enable TCR and TLR registers */
#define MCR_LOOP 	0x10 /* loop back */
#define MCR_XONANY 	0x20 /* enable XON Any */
#define MCR_IRDA	0x40 /* enable IrDA Mode */
#define MCR_CLKDIV4	0x80 /* enable divide-by-4 clock input */

/* constants for line status register */

#define LSR_RXRDY 	0x01 /* receiver data available */
#define LSR_OE 		0x02 /* overrun error */
#define LSR_PE 		0x04 /* parity error */
#define LSR_FE 		0x08 /* framing error */
#define LSR_BI 		0x10 /* break interrupt */
#define LSR_EOB_MASK 	0x1E /* Error or Break mask */
#define LSR_THRE 	0x20 /* transmit holding register empty */
#define LSR_TEMT 	0x40 /* transmitter empty */
#define LSR_FIFOERR	0x80 /* FIFO data error */

/* constants for modem status register */

#define MSR_DCTS 0x01 /* cts change */
#define MSR_CTS 0x10  /* complement of cts */

#define IIRC(dev) (((struct uart_sc16is741a_dev_data *)(dev)->data)->iir_cache)

/* device config */
struct uart_sc16is741a_device_config {
	i2c_dt_spec bus;
	uint32_t sys_clk_freq;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t	irq_config_func;
#endif
};

/** Device data structure */
struct uart_sc16is741a_dev_data {
	struct uart_config uart_config;
	struct k_spinlock lock;
	uint8_t fifo_size;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uint8_t iir_cache;	/**< cache of IIR since it clears when read */
	uart_irq_callback_user_data_t cb;  /**< Callback function pointer */
	void *cb_data;	/**< Callback function arg */
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && defined(CONFIG_PM)
	bool tx_stream_on;
#endif
};

static const struct uart_driver_api uart_sc16is741a_driver_api;

static uint8_t sc16is741a_read_reg(const struct device *dev, uint8_t regnum)
{
	uint8_t value;
	int res = i2c_write_read_dt(dev->bus, &regnum, 1, &value, 1);

	if (res) {
		return 0xFF;
	}
	return value;
}

static int sc16is741a_write_reg(const struct device *dev, uint8_t regnum, uint8_t value)
{
	uint8_t buf[2];

	buf[0] = regnum;
	buf[2] = value;
	return i2c_write_dt(dev->bus, buf, 2);
}


static void set_baud_rate(const struct device *dev, uint32_t baud_rate, uint32_t pclk)
{
	struct uart_sc16is741a_dev_data * const dev_data = dev->data;
	uint32_t divisor; /* baud rate divisor */
	uint8_t lcr_cache;

	if ((baud_rate != 0U) && (pclk != 0U)) {
		/*
		 * calculate baud rate divisor. a variant of
		 * (uint32_t)(pclk / (16.0 * baud_rate) + 0.5)
		 */
		divisor = ((pclk + (baud_rate << 3)) / baud_rate) >> 4;

		/* set the DLAB to access the baud rate divisor registers */
		lcr_cache = i2c_read_reg(dev, REG_LCR);
		i2c_write_reg(dev, REG_LCR, LCR_DLAB | lcr_cache);
		i2c_write_reg(dev, REG_DLL, (unsigned char)(divisor & 0xff));
		i2c_write_reg(dev, REG_DLH, (unsigned char)((divisor >> 8) & 0xff));

		/* restore the DLAB to access the baud rate divisor registers */
		i2c_write_reg(dev, REG_LCR, lcr_cache);

		dev_data->uart_config.baudrate = baud_rate;
	}
}

static int uart_sc16is741a_configure(const struct device *dev,
				  const struct uart_config *cfg)
{
	struct uart_sc16is741a_dev_data * const dev_data = dev->data;
	const struct uart_sc16is741a_device_config * const dev_cfg = dev->config;
	uint8_t mdc = 0U;
	uint32_t pclk = 0U;

	/* temp for return value if error occurs in this locked region */
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	ARG_UNUSED(dev_data);
	ARG_UNUSED(dev_cfg);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	dev_data->iir_cache = 0U;
#endif

	/*
	 * set clock frequency from clock_frequency property if valid,
	 * otherwise, get clock frequency from clock manager
	 */
	if (dev_cfg->sys_clk_freq != 0U) {
		pclk = dev_cfg->sys_clk_freq;
	} else {
		if (!device_is_ready(dev_cfg->clock_dev)) {
			ret = -EINVAL;
			goto out;
		}

		clock_control_get_rate(dev_cfg->clock_dev, dev_cfg->clock_subsys,
			   &pclk);
	}

	set_baud_rate(dev, cfg->baudrate, pclk);

	/* Local structure to hold temporary values to pass to OUTBYTE() */
	struct uart_config uart_cfg;

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		uart_cfg.data_bits = LCR_CS5;
		break;
	case UART_CFG_DATA_BITS_6:
		uart_cfg.data_bits = LCR_CS6;
		break;
	case UART_CFG_DATA_BITS_7:
		uart_cfg.data_bits = LCR_CS7;
		break;
	case UART_CFG_DATA_BITS_8:
		uart_cfg.data_bits = LCR_CS8;
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		uart_cfg.stop_bits = LCR_1_STB;
		break;
	case UART_CFG_STOP_BITS_2:
		uart_cfg.stop_bits = LCR_2_STB;
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		uart_cfg.parity = LCR_PDIS;
		break;
	case UART_CFG_PARITY_EVEN:
		uart_cfg.parity = LCR_EPS;
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	dev_data->uart_config = *cfg;

	/* data bits, stop bits, parity, clear DLAB */
	i2c_write_reg(dev, REG_LCR,
		uart_cfg.data_bits | uart_cfg.stop_bits | uart_cfg.parity);

	mdc = MCR_TLR_TCR | MCR_RTS;
#if defined(CONFIG_UART_SC16IS741A_VARIANT_NS16750) || \
	defined(CONFIG_UART_SC16IS741A_VARIANT_NS16950)
	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		mdc |= MCR_XONANY;
	}
#endif

	i2c_write_reg(dev, REG_MCR, mdc);

	/*
	 * Program FIFO: enabled, mode 0 (set for compatibility with quark),
	 * generate the interrupt at 8th byte
	 * Clear TX and RX FIFO
	 */
	i2c_write_reg(dev, REG_FCR,
		      (FCR_FIFO | FCR_MODE0 | FCR_TX_TRIG_8 | FCR_RX_TRIG_8 |
		       FCR_RCVRCLR | FCR_XMITCLR));

	if ((i2c_read_reg(dev, REG_IIR) & IIR_FE) == IIR_FE) {
		dev_data->fifo_size = 64;
	} else {
		dev_data->fifo_size = 1;
	}

	/* clear the port */
	i2c_read_reg(dev, REG_RHR);

	/* disable interrupts  */
	i2c_write_reg(dev, REG_IER, 0x00);

out:
	k_spin_unlock(&dev_data->lock, key);
	return ret;
};

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_sc16is741a_config_get(const struct device *dev,
				   struct uart_config *cfg)
{
	struct uart_sc16is741a_dev_data *data = dev->data;

	cfg->baudrate = data->uart_config.baudrate;
	cfg->parity = data->uart_config.parity;
	cfg->stop_bits = data->uart_config.stop_bits;
	cfg->data_bits = data->uart_config.data_bits;
	cfg->flow_ctrl = data->uart_config.flow_ctrl;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

/**
 * @brief Initialize individual UART port
 *
 * This routine is called to reset the chip in a quiescent state.
 *
 * @param dev UART device struct
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_sc16is741a_init(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	int ret;

	ret = uart_sc16is741a_configure(dev, &data->uart_config);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_sc16is741a_device_config *config = dev->config;

	config->irq_config_func(dev);
#endif

	return 0;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_sc16is741a_poll_in(const struct device *dev, unsigned char *c)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	int ret = -1;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if ((i2c_read_reg(dev, REG_LSR) & LSR_RXRDY) != 0) {
		/* got a character */
		*c = i2c_read_reg(dev, REG_RHR);
		ret = 0;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Output a character in polled mode.
 *
 * Checks if the transmitter is empty. If empty, a character is written to
 * the data register.
 *
 * If the hardware flow control is enabled then the handshake signal CTS has to
 * be asserted in order to send a character.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_sc16is741a_poll_out(const struct device *dev,
					   unsigned char c)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while ((i2c_read_reg(dev, REG_LSR) & LSR_THRE) == 0) {
	}

	i2c_write_reg(dev, REG_THR, c);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if an error was received
 *
 * @param dev UART device struct
 *
 * @return one of UART_ERROR_OVERRUN, UART_ERROR_PARITY, UART_ERROR_FRAMING,
 * UART_BREAK if an error was detected, 0 otherwise.
 */
static int uart_sc16is741a_err_check(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int check = i2c_read_reg(dev, REG_LSR) & LSR_EOB_MASK;

	k_spin_unlock(&data->lock, key);

	return check >> 1;
}

#if CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct
 * @param tx_data Data to transmit
 * @param size Number of bytes to send
 *
 * @return Number of bytes sent
 */
static int uart_sc16is741a_fifo_fill(const struct device *dev,
				     const uint8_t *tx_data,
				     int size)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	int i;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	for (i = 0; (i < size) && (i < data->fifo_size); i++) {
		i2c_write_reg(dev, REG_THR, tx_data[i]);
	}

	k_spin_unlock(&data->lock, key);

	return i;
}

/**
 * @brief Read data from FIFO
 *
 * @param dev UART device struct
 * @param rxData Data container
 * @param size Container size
 *
 * @return Number of bytes read
 */
static int uart_sc16is741a_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	int i;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	for (i = 0; (i < size) && (i2c_read_reg(dev, REG_LSR) & LSR_RXRDY) != 0; i++) {
		rx_data[i] = i2c_read_reg(dev, REG_RHR);
	}

	k_spin_unlock(&data->lock, key);

	return i;
}

/**
 * @brief Enable TX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_sc16is741a_irq_tx_enable(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && defined(CONFIG_PM)
	struct uart_sc16is741a_dev_data *const dev_data = dev->data;

	if (!dev_data->tx_stream_on) {
		dev_data->tx_stream_on = true;
		uint8_t num_cpu_states;
		const struct pm_state_info *cpu_states;

		num_cpu_states = pm_state_cpu_get_all(0U, &cpu_states);

		/*
		 * Power state to be disabled. Some platforms have multiple
		 * states and need to be given a constraint set according to
		 * different states.
		 */
		for (uint8_t i = 0U; i < num_cpu_states; i++) {
			pm_policy_state_lock_get(cpu_states[i].state, PM_ALL_SUBSTATES);
		}
	}
#endif
	i2c_write_reg(dev, REG_IER, i2c_read_reg(dev, REG_IER) | IER_TBE);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable TX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_sc16is741a_irq_tx_disable(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	i2c_write_reg(dev, REG_IER, i2c_read_reg(dev, REG_IER) & (~IER_TBE));

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && defined(CONFIG_PM)
	struct uart_sc16is741a_dev_data *const dev_data = dev->data;

	if (dev_data->tx_stream_on) {
		dev_data->tx_stream_on = false;
		uint8_t num_cpu_states;
		const struct pm_state_info *cpu_states;

		num_cpu_states = pm_state_cpu_get_all(0U, &cpu_states);

		/*
		 * Power state to be enabled. Some platforms have multiple
		 * states and need to be given a constraint release according
		 * to different states.
		 */
		for (uint8_t i = 0U; i < num_cpu_states; i++) {
			pm_policy_state_lock_put(cpu_states[i].state, PM_ALL_SUBSTATES);
		}
	}
#endif
	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_sc16is741a_irq_tx_ready(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = ((IIRC(dev) & IIR_ID) == IIR_THRE) ? 1 : 0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Check if nothing remains to be transmitted
 *
 * @param dev UART device struct
 *
 * @return 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_sc16is741a_irq_tx_complete(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = ((i2c_read_reg(dev, REG_LSR) & (LSR_TEMT | LSR_THRE))
				== (LSR_TEMT | LSR_THRE)) ? 1 : 0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Enable RX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_sc16is741a_irq_rx_enable(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	i2c_write_reg(dev, REG_IER, i2c_read_reg(dev, REG_IER) | IER_RXRDY);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable RX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_sc16is741a_irq_rx_disable(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	i2c_write_reg(dev, REG_IER, i2c_read_dev(dev, REG_IER) & (~IER_RXRDY));

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_sc16is741a_irq_rx_ready(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = ((IIRC(dev) & IIR_ID) == IIR_RBRF) ? 1 : 0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Enable error interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_sc16is741a_irq_err_enable(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	i2c_write_reg(dev, REG_IER, i2c_read_reg(dev, REG_IER) | IER_LSR);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable error interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static void uart_sc16is741a_irq_err_disable(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	i2c_write_data(dev, REG_IER, i2c_read_data(dev, REG_IER) & (~IER_LSR));

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_sc16is741a_irq_is_pending(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = (!(IIRC(dev) & IIR_NIP)) ? 1 : 0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Update cached contents of IIR
 *
 * @param dev UART device struct
 *
 * @return Always 1
 */
static int uart_sc16is741a_irq_update(const struct device *dev)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	IIRC(dev) = i2c_read_reg(dev, REG_IIR);

	k_spin_unlock(&data->lock, key);

	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 */
static void uart_sc16is741a_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb,
					  void *cb_data)
{
	struct uart_sc16is741a_dev_data * const dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 */
static void uart_sc16is741a_isr(const struct device *dev)
{
	struct uart_sc16is741a_dev_data * const dev_data = dev->data;

	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	}

#ifdef CONFIG_UART_SC16IS741A_WA_ISR_REENABLE_INTERRUPT
	uint8_t cached_ier = i2c_read_reg(dev, REG_IER);

	i2c_write_reg(dev, REG_IER, 0U);
	i2c_write_reg(dev, REG_IER, cached_ier);
#endif
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_SC16IS741A_LINE_CTRL

/**
 * @brief Manipulate line control for UART.
 *
 * @param dev UART device struct
 * @param ctrl The line control to be manipulated
 * @param val Value to set the line control
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_sc16is741a_line_ctrl_set(const struct device *dev,
				      uint32_t ctrl, uint32_t val)
{
	struct uart_sc16is741a_dev_data *data = dev->data;
	uint32_t mdc = 0, chg = 0;
	k_spinlock_key_t key;

	switch (ctrl) {
	case UART_LINE_CTRL_BAUD_RATE:
		set_baud_rate(dev, val);
		return 0;

	case UART_LINE_CTRL_RTS:
		key = k_spin_lock(&data->lock);
		mdc = i2c_read_reg(dev, REG_MDC);

		if (ctrl == UART_LINE_CTRL_RTS) {
			chg = MCR_RTS;
		} else {
			return -ENOTSUP;
		}

		if (val) {
			mdc |= chg;
		} else {
			mdc &= ~(chg);
		}

		i2c_write_reg(dev, REG_MDC, mdc);
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	return -ENOTSUP;
}

#endif /* CONFIG_UART_SC16IS741A_LINE_CTRL */


static const struct uart_driver_api uart_sc16is741a_driver_api = {
	.poll_in = uart_sc16is741a_poll_in,
	.poll_out = uart_sc16is741a_poll_out,
	.err_check = uart_sc16is741a_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_sc16is741a_configure,
	.config_get = uart_sc16is741a_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	.fifo_fill = uart_sc16is741a_fifo_fill,
	.fifo_read = uart_sc16is741a_fifo_read,
	.irq_tx_enable = uart_sc16is741a_irq_tx_enable,
	.irq_tx_disable = uart_sc16is741a_irq_tx_disable,
	.irq_tx_ready = uart_sc16is741a_irq_tx_ready,
	.irq_tx_complete = uart_sc16is741a_irq_tx_complete,
	.irq_rx_enable = uart_sc16is741a_irq_rx_enable,
	.irq_rx_disable = uart_sc16is741a_irq_rx_disable,
	.irq_rx_ready = uart_sc16is741a_irq_rx_ready,
	.irq_err_enable = uart_sc16is741a_irq_err_enable,
	.irq_err_disable = uart_sc16is741a_irq_err_disable,
	.irq_is_pending = uart_sc16is741a_irq_is_pending,
	.irq_update = uart_sc16is741a_irq_update,
	.irq_callback_set = uart_sc16is741a_irq_callback_set,

#endif

#ifdef CONFIG_UART_SC16IS741A_LINE_CTRL
	.line_ctrl_set = uart_sc16is741a_line_ctrl_set,
#endif
};

#define UART_SC16IS741A_IRQ_FLAGS_SENSE0(n) 0
#define UART_SC16IS741A_IRQ_FLAGS_SENSE1(n) DT_INST_IRQ(n, sense)
#define UART_SC16IS741A_IRQ_FLAGS(n) \
	_CONCAT(UART_SC16IS741A_IRQ_FLAGS_SENSE, DT_INST_IRQ_HAS_CELL(n, sense))(n)

#define UART_SC16IS741A_IRQ_CONFIG(n)                                         \
	static void irq_config_func##n(const struct device *dev)              \
	{                                                                     \
		ARG_UNUSED(dev);                                              \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	      \
			    uart_sc16is741a_isr, DEVICE_DT_INST_GET(n),	      \
			    UART_SC16IS741A_IRQ_FLAGS(n));		      \
		irq_enable(DT_INST_IRQN(n));                                  \
	}


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define DEV_CONFIG_IRQ_FUNC_INIT(n) \
	.irq_config_func = irq_config_func##n,
#define UART_SC16IS741A_IRQ_FUNC_DECLARE(n) \
	static void irq_config_func##n(const struct device *dev);
#define UART_SC16IS741A_IRQ_FUNC_DEFINE(n) \
	UART_SC16IS741A_IRQ_CONFIG(n)
#else
/* !CONFIG_UART_INTERRUPT_DRIVEN */
#define DEV_CONFIG_IRQ_FUNC_INIT(n)
#define UART_SC16IS741A_IRQ_FUNC_DECLARE(n)
#define UART_SC16IS741A_IRQ_FUNC_DEFINE(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define DEV_DATA_FLOW_CTRL0 UART_CFG_FLOW_CTRL_NONE
#define DEV_DATA_FLOW_CTRL1 UART_CFG_FLOW_CTRL_RTS_CTS
#define DEV_DATA_FLOW_CTRL(n) \
	_CONCAT(DEV_DATA_FLOW_CTRL, DT_INST_PROP_OR(n, hw_flow_control, 0))

#define UART_SC16IS741A_DEVICE_INIT(n)                                                  \
	UART_SC16IS741A_IRQ_FUNC_DECLARE(n);                                            \
	static const struct uart_sc16is741a_device_config uart_sc16is741a_dev_cfg_##n = {  \
		I2C_DT_SPEC_GET(DT_INST(n, DT_DRV_COMPAT)),				\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clock_frequency), (             \
				.sys_clk_freq = DT_INST_PROP(n, clock_frequency),    \
				.clock_dev = NULL,                                   \
				.clock_subsys = NULL,                                \
			), (                                                         \
				.sys_clk_freq = 0,                                   \
				.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),  \
				.clock_subsys = (clock_control_subsys_t) DT_INST_PHA(\
								0, clocks, clkid),   \
			)                                                            \
		)                                                                    \
		DEV_CONFIG_IRQ_FUNC_INIT(n)                                          \
	};                                                                           \
	static struct uart_sc16is741a_dev_data uart_sc16is741a_dev_data_##n = {            \
		.uart_config.baudrate = DT_INST_PROP_OR(n, current_speed, 0),        \
		.uart_config.parity = UART_CFG_PARITY_NONE,                          \
		.uart_config.stop_bits = UART_CFG_STOP_BITS_1,                       \
		.uart_config.data_bits = UART_CFG_DATA_BITS_8,                       \
		.uart_config.flow_ctrl = DEV_DATA_FLOW_CTRL(n),                      \
	};                                                                           \
	DEVICE_DT_INST_DEFINE(n, &uart_sc16is741a_init, NULL,                           \
			      &uart_sc16is741a_dev_data_##n, &uart_sc16is741a_dev_cfg_##n, \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,             \
			      &uart_sc16is741a_driver_api);                             \
	UART_SC16IS741A_IRQ_FUNC_DEFINE(n)

DT_INST_FOREACH_STATUS_OKAY(UART_SC16IS741A_DEVICE_INIT)
