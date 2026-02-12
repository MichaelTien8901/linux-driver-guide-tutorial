// SPDX-License-Identifier: GPL-2.0
/*
 * uio_userspace.c - User space program for UIO demo
 *
 * Compile: gcc -o uio_userspace uio_userspace.c
 * Run: sudo ./uio_userspace
 *
 * Opens /dev/uio0, mmaps the device memory region, and
 * reads/writes data to demonstrate user space hardware access.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

#define UIO_DEVICE "/dev/uio0"
#define MEM_SIZE 4096

int main(int argc, char *argv[])
{
	const char *dev_path = argc > 1 ? argv[1] : UIO_DEVICE;
	int fd;
	void *mem;
	char buf[64];

	printf("UIO User Space Demo\n");
	printf("====================\n\n");

	/* Open UIO device */
	fd = open(dev_path, O_RDWR);
	if (fd < 0) {
		perror("Failed to open UIO device");
		printf("Make sure uio_demo.ko is loaded\n");
		return 1;
	}
	printf("Opened %s\n", dev_path);

	/* Map device memory */
	mem = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE,
		   MAP_SHARED, fd, 0);
	if (mem == MAP_FAILED) {
		perror("mmap failed");
		close(fd);
		return 1;
	}
	printf("Mapped %d bytes of device memory\n\n", MEM_SIZE);

	/* Read identification data (written by kernel module) */
	memcpy(buf, mem, 8);
	buf[8] = '\0';
	printf("Test 1: Read identification\n");
	printf("  Device ID: '%s'\n\n", buf);

	/* Write data to device memory */
	printf("Test 2: Write data\n");
	const char *msg = "Hello from userspace!";
	memcpy((char *)mem + 64, msg, strlen(msg) + 1);
	printf("  Wrote: '%s' at offset 64\n\n", msg);

	/* Read back */
	printf("Test 3: Read back\n");
	memcpy(buf, (char *)mem + 64, strlen(msg) + 1);
	printf("  Read:  '%s' at offset 64\n\n", buf);

	/* Write structured data */
	printf("Test 4: Structured register access\n");
	volatile uint32_t *regs = (volatile uint32_t *)mem;
	regs[32] = 0xDEADBEEF;  /* Offset 128 */
	regs[33] = 0x12345678;  /* Offset 132 */
	printf("  Wrote: reg[32] = 0x%08X\n", regs[32]);
	printf("  Wrote: reg[33] = 0x%08X\n", regs[33]);
	printf("  Read:  reg[32] = 0x%08X\n", regs[32]);
	printf("  Read:  reg[33] = 0x%08X\n\n", regs[33]);

	/* Cleanup */
	munmap(mem, MEM_SIZE);
	close(fd);

	printf("All tests passed.\n");
	return 0;
}
