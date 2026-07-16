// SPDX-License-Identifier: GPL-2.0+
/*
 * ldddp-core.c - Subsystem implementation
 *
 * Copyright (C) 2026 Your Name <you@example.com>
 */

// Module init/exit macros (__init, __exit, module_init, module_exit)
#include <linux/init.h>
// Module metadata and symbol export (MODULE_*, EXPORT_SYMBOL)
#include <linux/module.h>
// Generic kernel logging (pr_info, pr_err)
#include <linux/printk.h>
// Memory allocation (kzalloc, kfree, GFP_KERNEL)
#include <linux/slab.h>
// LDDDP class API
#include "ldddp.h"

static struct class *ldddp_class;

static void ldddp_device_release(struct device *dev)
{
	struct ldddp_device *ldddp_dev = to_ldddp_device(dev);

	kfree(ldddp_dev);
}

static struct device_type ldddp_device_type = {
	.name = "ldddp_device",
	.release = ldddp_device_release,
};

struct ldddp_device *ldddp_allocate_device(struct device *parent)
{
	struct ldddp_device *ldddp_dev;
	static int counter = 0;
	int ret;

	ldddp_dev = kzalloc(sizeof(*ldddp_dev), GFP_KERNEL);
	if (!ldddp_dev)
		return ERR_PTR(-ENOMEM);

	ret = dev_set_name(&ldddp_dev->dev, "ldddp-%d", counter++);
	if (ret) {
		kfree(ldddp_dev);
		return ERR_PTR(ret);
	}

	ldddp_dev->dev.class = ldddp_class;
	ldddp_dev->dev.parent = parent;
	ldddp_dev->dev.type = &ldddp_device_type;

	return ldddp_dev;
}
EXPORT_SYMBOL(ldddp_allocate_device);

int ldddp_register_device(struct ldddp_device *ldddp_dev)
{
	int ret;

	if (!ldddp_dev)
		return -EINVAL;

	ret = device_register(&ldddp_dev->dev);
	if (ret) {
		dev_err(&ldddp_dev->dev, "failed to add class device: %d\n", ret);
		put_device(&ldddp_dev->dev);
		return ret;
	}

	// dev_info() will print dev_name(&ldddp_dev->dev) twice
	dev_info(&ldddp_dev->dev, "created class device '%s'\n",
		 dev_name(&ldddp_dev->dev));

	return 0;
}
EXPORT_SYMBOL(ldddp_register_device);

void ldddp_unregister_device(struct ldddp_device *ldddp_dev)
{
	if (!IS_ERR_OR_NULL(ldddp_dev)) {
		dev_info(&ldddp_dev->dev, "destroying class device\n");
		device_unregister(&ldddp_dev->dev);
	}
}
EXPORT_SYMBOL(ldddp_unregister_device);

static int __init ldddp_core_init(void)
{
	int ret;

	pr_info("ldddp-core: init\n");

	ldddp_class = class_create("ldddp");
	if (IS_ERR(ldddp_class)) {
		ret = PTR_ERR(ldddp_class);
		pr_err("ldddp-core: failed to create class: %d\n", ret);
		return ret;
	}

	pr_info("ldddp-core: ready (class: %s)\n", ldddp_class->name);

	return 0;
}

static void __exit ldddp_core_exit(void)
{
	pr_info("ldddp-core: exit\n");
	class_destroy(ldddp_class);
}

MODULE_AUTHOR("Your Name <you@example.com>");
MODULE_DESCRIPTION("LDDDP core to handle the LDDDP class");
MODULE_LICENSE("GPL");
module_init(ldddp_core_init);
module_exit(ldddp_core_exit);
