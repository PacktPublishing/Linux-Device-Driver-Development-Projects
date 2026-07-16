// SPDX-License-Identifier: GPL-2.0+
/*
 * simple-device.c - Device generator
 *
 * Copyright (C) 2026 Your Name <you@example.com>
 */
// Module init/exit macros (__init, __exit, module_init, module_exit)
#include <linux/init.h>
// Module metadata (MODULE_*)
#include <linux/module.h>
// Module parameters (module_param, MODULE_PARM_DESC)
#include <linux/moduleparam.h>
// Generic kernel logging (pr_info, pr_err)
#include <linux/printk.h>
// Simple bus types and helpers
#include "simple-bus.h"

static int dev_id = 1;

module_param(dev_id, int, 0444);
MODULE_PARM_DESC(dev_id, "Device ID");

static struct simple_device *sdev;

static int __init simple_device_init(void)
{
	sdev = simple_bus_allocate_device();
	if (IS_ERR(sdev))
		return PTR_ERR(sdev);

	sdev->id = dev_id;

	return simple_bus_register_device(sdev);
}

static void __exit simple_device_exit(void)
{
	pr_info("simple-device: exit\n");
	simple_bus_unregister_device(sdev);
}

MODULE_AUTHOR("Your Name <you@example.com>");
MODULE_DESCRIPTION("Trivial device for a simple bus");
MODULE_LICENSE("GPL");
module_init(simple_device_init);
module_exit(simple_device_exit);
