// SPDX-License-Identifier: GPL-2.0-only
/*
 * ldddp_iio_co2.c - Driver for the fictional LDDDP CO2 concentration sensor
 *
 * Copyright (C) 2026 Your Name <you@example.com>
 *
 * Register map:
 *
 *   0x00 ID     - read-only chip identifier
 *   0x01 CTRL   - bit 0: enable, bits [3:1]: output data rate select
 *   0x02 STATUS - bit 0: data-ready, cleared on read
 *   0x03 DATA_H - most significant byte of the last reading
 *   0x04 DATA_L - least significant byte of the last reading
 */

#include <linux/array_size.h>
#include <linux/bitfield.h>
#include <linux/bits.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>

#include <linux/iio/buffer.h>
#include <linux/iio/iio.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>

#define LDDDP_REG_ID			0x00
#define LDDDP_REG_CTRL			0x01
#define LDDDP_REG_STATUS		0x02
#define LDDDP_REG_DATA_H		0x03
#define LDDDP_REG_DATA_L		0x04

#define LDDDP_CHIP_ID			0x07

#define LDDDP_CTRL_ENABLE		BIT(0)
#define LDDDP_CTRL_ODR_MASK		GENMASK(3, 1)

#define LDDDP_STATUS_DRDY		BIT(0)

static const int ldddp_sampling_freq[] = { 1, 2, 5, 10, 25, 50, 100 };

struct ldddp_priv {
	struct regmap *regmap;
};

static const struct regmap_range ldddp_volatile_ranges[] = {
	regmap_reg_range(LDDDP_REG_STATUS, LDDDP_REG_DATA_L),
};

static const struct regmap_access_table ldddp_volatile_table = {
	.yes_ranges = ldddp_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(ldddp_volatile_ranges),
};

static bool ldddp_precious_reg(struct device *dev, unsigned int reg)
{
	return reg == LDDDP_REG_STATUS;
}

static const struct regmap_config ldddp_regmap_config = {
	.name = "ldddp_co2_sensor",
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = LDDDP_REG_DATA_L,
	.cache_type = REGCACHE_MAPLE,
	.volatile_table = &ldddp_volatile_table,
	.precious_reg = ldddp_precious_reg,
};

static const struct iio_chan_spec ldddp_channels[] = {
	{
		.type = IIO_CONCENTRATION,
		.modified = 1,
		.channel2 = IIO_MOD_CO2,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
				       BIT(IIO_CHAN_INFO_SCALE) |
				       BIT(IIO_CHAN_INFO_SAMP_FREQ),
		.scan_index = 0,
		.scan_type = {
			.format = IIO_SCAN_FORMAT_UNSIGNED_INT,
			.realbits = 16,
			.storagebits = 16,
			.endianness = IIO_CPU,
		},
	},
	IIO_CHAN_SOFT_TIMESTAMP(1),
};

static int ldddp_read_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *chan,
			   int *val, int *val2, long mask)
{
	struct ldddp_priv *priv = iio_priv(indio_dev);
	unsigned int status, ctrl, odr;
	u8 data[2];
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW: {
		IIO_DEV_ACQUIRE_DIRECT_MODE(indio_dev, claim);
		if (IIO_DEV_ACQUIRE_FAILED(claim))
			return -EBUSY;

		ret = regmap_read(priv->regmap, LDDDP_REG_STATUS, &status);
		if (ret)
			return ret;

		if (!(status & LDDDP_STATUS_DRDY))
			return -EBUSY;

		ret = regmap_bulk_read(priv->regmap, LDDDP_REG_DATA_H,
					data, sizeof(data));
		if (ret)
			return ret;

		*val = (data[0] << 8) | data[1];
		return IIO_VAL_INT;
	}
	case IIO_CHAN_INFO_SCALE:
		*val = 0;
		*val2 = 100000;
		return IIO_VAL_INT_PLUS_MICRO;
	case IIO_CHAN_INFO_SAMP_FREQ: {
		IIO_DEV_ACQUIRE_DIRECT_MODE(indio_dev, claim);
		if (IIO_DEV_ACQUIRE_FAILED(claim))
			return -EBUSY;

		ret = regmap_read(priv->regmap, LDDDP_REG_CTRL, &ctrl);
		if (ret)
			return ret;

		odr = FIELD_GET(LDDDP_CTRL_ODR_MASK, ctrl);
		if (odr >= ARRAY_SIZE(ldddp_sampling_freq))
			return -EINVAL;

		*val = ldddp_sampling_freq[odr];
		return IIO_VAL_INT;
	}
	default:
		return -EINVAL;
	}
}

static int ldddp_sampling_freq_to_odr(int freq)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(ldddp_sampling_freq); i++)
		if (ldddp_sampling_freq[i] == freq)
			return i;

	return -EINVAL;
}

static int ldddp_write_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int val, int val2, long mask)
{
	struct ldddp_priv *priv = iio_priv(indio_dev);
	int data_rate;

	switch (mask) {
	case IIO_CHAN_INFO_SAMP_FREQ: {
		if (val2)
			return -EINVAL;

		data_rate = ldddp_sampling_freq_to_odr(val);
		if (data_rate < 0)
			return data_rate;

		IIO_DEV_ACQUIRE_DIRECT_MODE(indio_dev, claim);
		if (IIO_DEV_ACQUIRE_FAILED(claim))
			return -EBUSY;

		return regmap_update_bits(priv->regmap, LDDDP_REG_CTRL,
					  LDDDP_CTRL_ODR_MASK,
					  FIELD_PREP(LDDDP_CTRL_ODR_MASK, data_rate));
	}
	default:
		return -EINVAL;
	}
}

static const struct iio_info ldddp_info = {
	.read_raw = ldddp_read_raw,
	.write_raw = ldddp_write_raw,
};

static irqreturn_t ldddp_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct ldddp_priv *priv = iio_priv(indio_dev);
	IIO_DECLARE_BUFFER_WITH_TS(u16, scan, 1) = { };
	unsigned int status;
	u8 data[2];
	int ret;

	ret = regmap_read(priv->regmap, LDDDP_REG_STATUS, &status);
	if (ret || !(status & LDDDP_STATUS_DRDY))
		goto done;

	ret = regmap_bulk_read(priv->regmap, LDDDP_REG_DATA_H, data, sizeof(data));
	if (ret)
		goto done;

	scan[0] = (data[0] << 8) | data[1];

	iio_push_to_buffers_with_ts(indio_dev, scan, sizeof(scan),
				    pf->timestamp);

done:
	iio_trigger_notify_done(indio_dev->trig);
	return IRQ_HANDLED;
}

static int ldddp_probe(struct i2c_client *client)
{
	struct ldddp_priv *priv;
	struct iio_dev *indio_dev;
	struct regmap *regmap;
	unsigned int id;
	int ret;

	regmap = devm_regmap_init_i2c(client, &ldddp_regmap_config);
	if (IS_ERR(regmap))
		return dev_err_probe(&client->dev, PTR_ERR(regmap),
				      "failed to initialize regmap\n");

	ret = regmap_read(regmap, LDDDP_REG_ID, &id);
	if (ret)
		return ret;

	if (id != LDDDP_CHIP_ID)
		return dev_err_probe(&client->dev, -ENODEV,
				      "unexpected chip id 0x%02x\n", id);

	ret = regmap_write(regmap, LDDDP_REG_CTRL, LDDDP_CTRL_ENABLE);
	if (ret)
		return ret;

	/*
	 * A cleanup action to clear LDDDP_CTRL_ENABLE and power down the
	 * device would be necessary. We will cover that in a later chapter.
	 */

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*priv));
	if (!indio_dev)
		return -ENOMEM;

	priv = iio_priv(indio_dev);
	priv->regmap = regmap;

	indio_dev->name = "ldddp_co2_sensor";
	indio_dev->info = &ldddp_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = ldddp_channels;
	indio_dev->num_channels = ARRAY_SIZE(ldddp_channels);

	ret = devm_iio_triggered_buffer_setup(&client->dev, indio_dev,
					      iio_pollfunc_store_time,
					      ldddp_trigger_handler, NULL);
	if (ret)
		return ret;

	return devm_iio_device_register(&client->dev, indio_dev);
}

static const struct i2c_device_id ldddp_id_table[] = {
	{ "ldddp_co2_sensor" },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ldddp_id_table);

static const struct of_device_id ldddp_of_match[] = {
	{ .compatible = "ldddp,ldddp-co2-sensor" },
	{ }
};
MODULE_DEVICE_TABLE(of, ldddp_of_match);

static struct i2c_driver ldddp_driver = {
	.driver = {
		.name = "ldddp_co2_sensor",
		.of_match_table = ldddp_of_match,
	},
	.probe = ldddp_probe,
	.id_table = ldddp_id_table,
};
module_i2c_driver(ldddp_driver);

MODULE_AUTHOR("Javier Carrasco <javier.carrasco@ldddp.com>");
MODULE_DESCRIPTION("LDDDP IIO CO2 concentration sensor driver");
MODULE_LICENSE("GPL");
