/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * ldddp.h - header file for the LDDDP class (subsystem)
 */
#ifndef _LDDDP_H
#define _LDDDP_H

// Device model core types (struct device)
#include <linux/device.h>

struct ldddp_device {
	struct device dev;
};

struct ldddp_device *ldddp_allocate_device(struct device *parent);
int ldddp_register_device(struct ldddp_device *ldddp_dev);
void ldddp_unregister_device(struct ldddp_device *ldddp_dev);

static inline struct ldddp_device *to_ldddp_device(struct device *dev)
{
	return container_of(dev, struct ldddp_device, dev);
}

#endif /* _LDDDP_H */
