/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * simple-bus.h - header file for the simple bus
 */
#ifndef _SIMPLE_BUS_H
#define _SIMPLE_BUS_H
// Device model core types (struct device, struct device_driver, bus_type)
#include <linux/device.h>
// module_driver macro used in module_simple_driver
#include <linux/module.h>

struct simple_device {
	struct device dev;
	const char *name;
	int id;
};

struct simple_driver {
	int (*probe)(struct simple_device *sdev);
	void (*remove)(struct simple_device *sdev);
	struct device_driver driver;
};

struct simple_device *simple_bus_allocate_device(void);
int simple_bus_register_device(struct simple_device *sdev);
void simple_bus_unregister_device(struct simple_device *sdev);
int simple_bus_register_driver(struct simple_driver *drv);
void simple_bus_unregister_driver(struct simple_driver *drv);

static inline struct simple_device *to_simple_device(struct device *dev)
{
	return container_of(dev, struct simple_device, dev);
}

static inline struct simple_driver *to_simple_driver(struct device_driver *drv)
{
	return container_of(drv, struct simple_driver, driver);
}

#define module_simple_driver(__simple_driver) \
	module_driver(__simple_driver, simple_bus_register_driver, \
		simple_bus_unregister_driver)

#endif /* _SIMPLE_BUS_H */
