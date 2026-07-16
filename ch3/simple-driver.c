// SPDX-License-Identifier: GPL-2.0+
/*
 * simple-driver.c - Driver for a simple LDDDP device
 *
 * Copyright (C) 2026 Your Name <you@example.com>
 *
 */
// Simple bus types and helpers
#include "simple-bus.h"
// Module metadata (MODULE_*)
#include <linux/module.h>
// Memory allocation (kzalloc, kfree, GFP_KERNEL)
#include <linux/slab.h>
// LDDDP class API
#include "ldddp.h"

struct simple_drv_priv {
	struct ldddp_device *ldddp_dev;
	int device_id;
};

static int simple_probe(struct simple_device *sdev)
{
	struct simple_drv_priv *priv;
	struct ldddp_device *ldddp_dev;
	struct device *dev = &sdev->dev;
	int ret;

	ldddp_dev = ldddp_allocate_device(dev);
	if (IS_ERR(ldddp_dev)) {
		ret = PTR_ERR(ldddp_dev);
		dev_err(dev, "failed to allocate LDDDP device: %d\n", ret);
		return ret;
	}

	ret = ldddp_register_device(ldddp_dev);
	if (ret) {
		dev_err(dev, "failed to register LDDDP device: %d\n", ret);
		return ret;
	}

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		ldddp_unregister_device(ldddp_dev);
		return -ENOMEM;
	}

	priv->ldddp_dev = ldddp_dev;
	priv->device_id = sdev->id % 2 == 0 ? 0 : 1;
	dev_set_drvdata(dev, priv);

	dev_info(dev, "Registered %s LDDDP device\n",
		 priv->device_id ? "odd" : "even");

	return 0;
}

static void simple_remove(struct simple_device *sdev)
{
	struct simple_drv_priv *priv = dev_get_drvdata(&sdev->dev);

	if (priv) {
		if (priv->ldddp_dev)
			ldddp_unregister_device(priv->ldddp_dev);

		kfree(priv);
	}
}

static struct simple_driver simple_drv = {
	.probe = simple_probe,
	.remove = simple_remove,
	.driver = {
		.name = "simple-device", // device name -> simple matching
	},
};
module_simple_driver(simple_drv);

MODULE_AUTHOR("Your Name <you@example.com>");
MODULE_DESCRIPTION("driver for a simple device on a simple bus");
MODULE_LICENSE("GPL");
