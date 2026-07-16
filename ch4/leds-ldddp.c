// SPDX-License-Identifier: GPL-2.0
/*
 * LDDDP LED controller driver
 *
 * Copyright (C) 2026 Your Name <you@example.com>
 *
 * Drives up to 4 LEDs via MMIO registers. Each LED occupies one u32
 * register at offset (index * 4) from the controller base address.
 * The register holds the brightness value directly (0-255).
 *
 * Register map:
 *   0x00  - LED 0 brightness
 *   0x04  - LED 1 brightness
 *   0x08  - LED 2 brightness
 *   0x0C  - LED 3 brightness
 */

#include <linux/device.h>
#include <linux/io.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>

struct ldddp_led {
	struct led_classdev cdev;
	void __iomem *reg;	/* pointer to this LED's register */
};

struct ldddp_priv {
	void __iomem *base;
	unsigned int num_leds;
	/* flexible array, one per child node */
	struct ldddp_led leds[] __counted_by(num_leds);
};

static void ldddp_brightness_set(struct led_classdev *cdev,
				 enum led_brightness brightness)
{
	struct ldddp_led *led = container_of(cdev, struct ldddp_led, cdev);

	writel(brightness, led->reg);
}

static enum led_brightness ldddp_brightness_get(struct led_classdev *cdev)
{
	struct ldddp_led *led = container_of(cdev, struct ldddp_led, cdev);

	return readl(led->reg);
}

static int ldddp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ldddp_priv *priv;
	struct resource *res;
	unsigned int i = 0, num_leds;
	int ret;
	u32 reg;

	num_leds = device_get_child_node_count(dev);
	if (!num_leds)
		return dev_err_probe(dev, -ENODEV, "no LED child nodes found\n");

	priv = devm_kzalloc(dev, struct_size(priv, leds, num_leds), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->num_leds = num_leds;
	priv->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	/*
	 * Fetched separately (instead of relying only on
	 * devm_platform_ioremap_resource()) so we know the size of the
	 * mapped region and can validate each child's 'reg' against it,
	 * preventing out-of-bounds MMIO accesses below.
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;

	device_for_each_child_node_scoped(dev, child) {
		struct led_init_data init_data;
		enum led_default_state default_state;
		u32 max_brightness;

		init_data.fwnode = child;

		ret = fwnode_property_read_u32(child, "reg", &reg);
		if (ret)
			return dev_err_probe(dev, ret,
					     "LED node missing 'reg'\n");

		/* Reject reg if it falls outside the valid range */
		if ((u64)(reg + 1) * sizeof(u32) > resource_size(res))
			return dev_err_probe(dev, -EINVAL,
					     "LED %u: reg out of controller range\n",
					     reg);

		priv->leds[i].reg = priv->base + reg * sizeof(u32);
		priv->leds[i].cdev.brightness_set = ldddp_brightness_set;
		priv->leds[i].cdev.brightness_get = ldddp_brightness_get;

		default_state = led_init_default_state_get(child);
		switch (default_state) {
		case LEDS_DEFSTATE_ON:
			ret = fwnode_property_read_u32(child, "max-brightness",
						       &max_brightness);
			if (ret)
				return dev_err_probe(dev, ret,
						     "LED %u: default-state=on requires max-brightness\n",
						     reg);

			writel(max_brightness, priv->leds[i].reg);
			priv->leds[i].cdev.brightness = max_brightness;
			break;
		case LEDS_DEFSTATE_KEEP:
			priv->leds[i].cdev.brightness =
				readl(priv->leds[i].reg);
			break;
		case LEDS_DEFSTATE_OFF:
		default:
			writel(LED_OFF, priv->leds[i].reg);
			priv->leds[i].cdev.brightness = LED_OFF;
			break;
		}

		ret = devm_led_classdev_register_ext(dev, &priv->leds[i].cdev,
						     &init_data);
		if (ret)
			return dev_err_probe(dev, ret,
					     "failed to register LED %u\n", reg);

		dev_info(dev, "registered LED at offset 0x%02x\n",
			 reg * (unsigned int)sizeof(u32));
		i++;
	}

	return 0;
}

static const struct of_device_id ldddp_led_of_match[] = {
	{ .compatible = "ldddp,led-ctrl" },
	{ }
};
MODULE_DEVICE_TABLE(of, ldddp_led_of_match);

static struct platform_driver ldddp_led_driver = {
	.probe = ldddp_probe,
	.driver = {
		.name = "ldddp-led",
		.of_match_table	= ldddp_led_of_match,
	},
};
module_platform_driver(ldddp_led_driver);

MODULE_AUTHOR("Your Name <you@example.com>");
MODULE_DESCRIPTION("LDDDP LED controller driver");
MODULE_LICENSE("GPL");
