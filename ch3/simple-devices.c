// SPDX-License-Identifier: GPL-2.0+
/*
 * simple-devices.c - Multi-device generator
 *
 * Copyright (C) 2026 Your Name <you@example.com>
 *
 */
// Module init/exit macros (__init, __exit, module_init, module_exit)
#include <linux/init.h>
// Module metadata (MODULE_*)
#include <linux/module.h>
// Module parameters (module_param_array, MODULE_PARM_DESC)
#include <linux/moduleparam.h>
// Generic kernel logging (pr_info, pr_err)
#include <linux/printk.h>
// Simple bus types and helpers
#include "simple-bus.h"

static int dev_ids[8];
static int dev_ids_count;

module_param_array(dev_ids, int, &dev_ids_count, 0444);
MODULE_PARM_DESC(dev_ids, "Device IDs (comma-separated)");

static struct simple_device *sdevs[8];

static int __init simple_device_init(void)
{
	int ret, i;

	if (!dev_ids_count) {
		pr_err("simple-device: at least one device ID is required\n");
		return -EINVAL;
	}

	for (i = 0; i < dev_ids_count; i++) {
		sdevs[i] = simple_bus_allocate_device();
		if (IS_ERR(sdevs[i])) {
			ret = PTR_ERR(sdevs[i]);
			goto err;
		}

		sdevs[i]->name = "simple-device";
		sdevs[i]->id = dev_ids[i];

		ret = simple_bus_register_device(sdevs[i]);
		if (ret)
			goto err;
	}

	return 0;
err:
	while (--i >= 0)
		simple_bus_unregister_device(sdevs[i]);
	return ret;
}

static void __exit simple_device_exit(void)
{
	int i;

	pr_info("simple-device: exit\n");
	for (i = 0; i < dev_ids_count; i++)
		simple_bus_unregister_device(sdevs[i]);
}

MODULE_AUTHOR("Your Name <you@example.com>");
MODULE_DESCRIPTION("Trivial device for a simple bus");
MODULE_LICENSE("GPL");
module_init(simple_device_init);
module_exit(simple_device_exit);
