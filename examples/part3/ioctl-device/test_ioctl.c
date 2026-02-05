// SPDX-License-Identifier: GPL-2.0
/*
 * test_ioctl.c - User space test program for ioctl_example driver
 *
 * Compile: gcc -o test_ioctl test_ioctl.c
 * Run: sudo ./test_ioctl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "ioctl_example.h"

#define DEVICE_PATH "/dev/ioctl_example"

void print_stats(struct ioctl_stats *stats)
{
	printf("Statistics:\n");
	printf("  Reads:      %lu\n", stats->reads);
	printf("  Writes:     %lu\n", stats->writes);
	printf("  IOCTLs:     %lu\n", stats->ioctls);
	printf("  Last error: %d\n", stats->last_error);
}

void print_config(struct ioctl_config *config)
{
	printf("Configuration:\n");
	printf("  Speed: %d\n", config->speed);
	printf("  Mode:  %d\n", config->mode);
	printf("  Name:  %s\n", config->name);
}

int main(int argc, char *argv[])
{
	int fd;
	int ret;
	int value;
	struct ioctl_config config;
	struct ioctl_stats stats;

	printf("IOCTL Example Test Program\n");
	printf("==========================\n\n");

	/* Open the device */
	fd = open(DEVICE_PATH, O_RDWR);
	if (fd < 0) {
		perror("Failed to open device");
		return 1;
	}

	printf("Device opened successfully\n\n");

	/* Test 1: RESET */
	printf("Test 1: RESET command\n");
	ret = ioctl(fd, IOCTL_RESET);
	if (ret < 0) {
		perror("IOCTL_RESET failed");
	} else {
		printf("  RESET successful\n");
	}
	printf("\n");

	/* Test 2: SET_CONFIG */
	printf("Test 2: SET_CONFIG command\n");
	config.speed = 100;
	config.mode = 2;
	strncpy(config.name, "test_device", sizeof(config.name));

	ret = ioctl(fd, IOCTL_SET_CONFIG, &config);
	if (ret < 0) {
		perror("IOCTL_SET_CONFIG failed");
	} else {
		printf("  Configuration set:\n");
		printf("    Speed: %d\n", config.speed);
		printf("    Mode:  %d\n", config.mode);
		printf("    Name:  %s\n", config.name);
	}
	printf("\n");

	/* Test 3: SET_VALUE */
	printf("Test 3: SET_VALUE command\n");
	value = 42;
	ret = ioctl(fd, IOCTL_SET_VALUE, &value);
	if (ret < 0) {
		perror("IOCTL_SET_VALUE failed");
	} else {
		printf("  Value set to: %d\n", value);
	}
	printf("\n");

	/* Test 4: GET_VALUE */
	printf("Test 4: GET_VALUE command\n");
	value = 0;
	ret = ioctl(fd, IOCTL_GET_VALUE, &value);
	if (ret < 0) {
		perror("IOCTL_GET_VALUE failed");
	} else {
		printf("  Value read: %d\n", value);
	}
	printf("\n");

	/* Test 5: GET_STATS */
	printf("Test 5: GET_STATS command\n");
	ret = ioctl(fd, IOCTL_GET_STATS, &stats);
	if (ret < 0) {
		perror("IOCTL_GET_STATS failed");
	} else {
		print_stats(&stats);
	}
	printf("\n");

	/* Test 6: XFER_CONFIG */
	printf("Test 6: XFER_CONFIG command (bidirectional)\n");
	memset(&config, 0, sizeof(config));
	config.speed = 999;  /* This will be overwritten with current config */
	config.mode = 0;
	strncpy(config.name, "new_name", sizeof(config.name));

	ret = ioctl(fd, IOCTL_XFER_CONFIG, &config);
	if (ret < 0) {
		perror("IOCTL_XFER_CONFIG failed");
	} else {
		printf("  Received current configuration:\n");
		print_config(&config);
	}
	printf("\n");

	/* Test 7: Invalid command */
	printf("Test 7: Invalid command (should fail)\n");
	ret = ioctl(fd, _IO(IOCTL_MAGIC, 99));
	if (ret < 0) {
		printf("  Expected error: %s\n", strerror(errno));
	} else {
		printf("  Unexpected success!\n");
	}
	printf("\n");

	/* Test 8: Invalid configuration */
	printf("Test 8: Invalid configuration (should fail)\n");
	config.speed = 9999;  /* Invalid: > 1000 */
	config.mode = 0;
	strncpy(config.name, "invalid", sizeof(config.name));

	ret = ioctl(fd, IOCTL_SET_CONFIG, &config);
	if (ret < 0) {
		printf("  Expected error: %s\n", strerror(errno));
	} else {
		printf("  Unexpected success!\n");
	}
	printf("\n");

	/* Final stats */
	printf("Final Statistics:\n");
	ret = ioctl(fd, IOCTL_GET_STATS, &stats);
	if (ret == 0) {
		print_stats(&stats);
	}

	close(fd);
	printf("\nDevice closed. All tests complete.\n");

	return 0;
}
