// SPDX-License-Identifier: GPL-2.0+
/*
 * simple-bus.c - Simple bus implementation to provide matching and uevents
 */
// Module init/exit macros (__init, __exit, module_init, module_exit)
#include <linux/init.h>
// Uevent environment (struct kobj_uevent_env, add_uevent_var)
#include <linux/kobject.h>
// Module metadata and symbol export (MODULE_*, EXPORT_SYMBOL)
#include <linux/module.h>
// Generic kernel logging (pr_info, pr_err)
#include <linux/printk.h>
// Memory allocation (kzalloc, kfree)
#include <linux/slab.h>
// String comparison (strcmp)
#include <linux/string.h>
// Simple bus types and helpers
#include "simple-bus.h"

static int simple_bus_match(struct device *dev, const struct device_driver *drv)
{
	struct simple_device *sdev = to_simple_device(dev);

	pr_info("simple-bus: trying to match '%s' with driver '%s'\n",
		dev_name(dev), drv->name);

	return strcmp(sdev->name, drv->name) == 0;
}

static int simple_bus_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
	int ret;

	ret = add_uevent_var(env, "DEVICE=%s", dev_name(dev));
	if (ret)
		return ret;

	if (dev->driver) {
		ret = add_uevent_var(env, "DRIVER=%s",
				     dev->driver->name);
		if (ret)
			return ret;
	} else {
		ret = add_uevent_var(env, "DRIVER=none");
		if (ret)
			return ret;
	}

	return 0;
}

static int simple_bus_probe(struct device *dev)
{
	struct simple_driver *drv;

	if (!dev->driver)
		return 0;

	drv = to_simple_driver(dev->driver);

	if (drv->probe)
		return drv->probe(to_simple_device(dev));

	return 0;
}

static void simple_bus_remove(struct device *dev)
{
	struct simple_driver *drv;

	if (!dev->driver)
		return;

	drv = to_simple_driver(dev->driver);

	if (drv->remove)
		drv->remove(to_simple_device(dev));
}

static const struct bus_type simple_bus = {
	.name = "simple-bus",
	.match = simple_bus_match,
	.uevent = simple_bus_uevent,
	.probe = simple_bus_probe,
	.remove = simple_bus_remove,
};

/* Static device, no dynamic allocation to free */
static void simple_bus_controller_release(struct device *dev)
{
}

static struct device simple_bus_controller = {
	.init_name = "simple-bus-controller",
	.release = simple_bus_controller_release,
};

static void simple_device_release(struct device *dev)
{
	struct simple_device *sdev = to_simple_device(dev);

	dev_info(dev, "releasing device '%s', ID = %d\n", sdev->name, sdev->id);
	kfree(sdev);
}

struct simple_device *simple_bus_allocate_device(void)
{
	struct simple_device *sdev;

	sdev = kzalloc(sizeof(*sdev), GFP_KERNEL);
	if (!sdev)
		return ERR_PTR(-ENOMEM);

	device_initialize(&sdev->dev);
	sdev->name = "simple-device";
	sdev->dev.bus = &simple_bus;
	sdev->dev.parent = &simple_bus_controller;
	sdev->dev.release = simple_device_release;

	return sdev;
}
EXPORT_SYMBOL(simple_bus_allocate_device);

int simple_bus_register_device(struct simple_device *sdev)
{
	int ret;

	if (!sdev)
		return -EINVAL;

	ret = dev_set_name(&sdev->dev, "%s.%d", sdev->name, sdev->id);
	if (ret) {
		/* pr_err() because sdev->dev does not have a name */
		pr_err("simple-bus: failed to set device name '%s.%d': %d\n",
		       sdev->name, sdev->id, ret);
		goto put_device;
	}

	ret = device_add(&sdev->dev);
	if (ret) {
		/* dev_err() is preferred now that the name is available */
		dev_err(&sdev->dev, "failed to add device: %d\n", ret);
		goto put_device;
	}

	dev_info(&sdev->dev, "registered device (ID = %d)\n", sdev->id);

	return 0;

put_device:
	put_device(&sdev->dev);
	return ret;
}
EXPORT_SYMBOL(simple_bus_register_device);

void simple_bus_unregister_device(struct simple_device *sdev)
{
	if (sdev)
		device_unregister(&sdev->dev);
}
EXPORT_SYMBOL(simple_bus_unregister_device);

int simple_bus_register_driver(struct simple_driver *drv)
{
	if (!drv)
		return -EINVAL;

	drv->driver.bus = &simple_bus;

	return driver_register(&drv->driver);
}
EXPORT_SYMBOL(simple_bus_register_driver);

void simple_bus_unregister_driver(struct simple_driver *drv)
{
	if (drv)
		driver_unregister(&drv->driver);
}
EXPORT_SYMBOL(simple_bus_unregister_driver);

static int __init simple_bus_init(void)
{
	int ret;

	pr_info("simple-bus: init\n");

	/* Register the bus controller device */
	ret = device_register(&simple_bus_controller);
	if (ret) {
		pr_err("simple-bus: failed to register bus controller: %d\n", ret);
		return ret;
	}

	pr_info("simple-bus: bus controller device registered\n");

	/* Now register the bus type */
	ret = bus_register(&simple_bus);
	if (ret) {
		pr_err("simple-bus: failed to register bus: %d\n", ret);
		device_unregister(&simple_bus_controller);
		return ret;
	}

	pr_info("simple-bus: ready\n");

	return 0;
}

static void __exit simple_bus_exit(void)
{
	pr_info("simple-bus: exit\n");
	bus_unregister(&simple_bus);
	device_unregister(&simple_bus_controller);
}

MODULE_AUTHOR("Your Name <you@example.com>");
MODULE_DESCRIPTION("LDDDP Trivial bus");
MODULE_LICENSE("GPL");
module_init(simple_bus_init);
module_exit(simple_bus_exit);
