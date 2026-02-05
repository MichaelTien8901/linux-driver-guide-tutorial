/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ioctl_example.h - Shared IOCTL definitions
 *
 * This header is shared between kernel module and user space applications.
 */

#ifndef IOCTL_EXAMPLE_H
#define IOCTL_EXAMPLE_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

/* Magic number for our driver */
#define IOCTL_MAGIC 'E'

/* Data structures for IOCTL commands */
struct ioctl_config {
	int speed;
	int mode;
	char name[32];
};

struct ioctl_stats {
	unsigned long reads;
	unsigned long writes;
	unsigned long ioctls;
	int last_error;
};

/* IOCTL command definitions */

/* No data transfer */
#define IOCTL_RESET         _IO(IOCTL_MAGIC, 0)

/* Read from driver (driver writes to user) */
#define IOCTL_GET_STATS     _IOR(IOCTL_MAGIC, 1, struct ioctl_stats)
#define IOCTL_GET_VALUE     _IOR(IOCTL_MAGIC, 2, int)

/* Write to driver (driver reads from user) */
#define IOCTL_SET_CONFIG    _IOW(IOCTL_MAGIC, 3, struct ioctl_config)
#define IOCTL_SET_VALUE     _IOW(IOCTL_MAGIC, 4, int)

/* Read and write */
#define IOCTL_XFER_CONFIG   _IOWR(IOCTL_MAGIC, 5, struct ioctl_config)

/* Maximum command number for validation */
#define IOCTL_MAXNR 5

#endif /* IOCTL_EXAMPLE_H */
