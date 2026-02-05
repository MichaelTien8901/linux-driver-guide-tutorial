# Simple Character Device Example

A basic character device driver demonstrating fundamental file operations.

## Files

- `simple_char.c` - Character device driver
- `Makefile` - Build configuration

## Features Demonstrated

1. **Device Registration**
   - `alloc_chrdev_region()` - Dynamic device number allocation
   - `cdev_init()` / `cdev_add()` - Character device setup
   - `class_create()` / `device_create()` - Automatic /dev node creation

2. **File Operations**
   - `open()` - Device open handler
   - `release()` - Device close handler
   - `read()` - Read data from device
   - `write()` - Write data to device
   - `llseek()` - Seek to position

3. **Data Transfer**
   - `copy_to_user()` - Send data to user space
   - `copy_from_user()` - Receive data from user space

4. **Synchronization**
   - Mutex protection for concurrent access

## Building

```bash
# Native build
make

# Cross-compile for ARM64
make KERNEL_DIR=/path/to/kernel ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

## Testing

### Load the Module

```bash
sudo insmod simple_char.ko
dmesg | tail -3
# simple_char: registered with major=243, minor=0
```

### Verify Device Node

```bash
ls -la /dev/simple_char
# crw------- 1 root root 243, 0 Jan 15 10:00 /dev/simple_char
```

### Basic Read/Write

```bash
# Write to device
echo "Hello, driver!" | sudo tee /dev/simple_char

# Read from device (resets position first)
sudo cat /dev/simple_char
# Hello, driver!
```

### Using dd for Precise Control

```bash
# Write at specific offset
echo "test" | sudo dd of=/dev/simple_char bs=1 seek=10

# Read specific bytes
sudo dd if=/dev/simple_char bs=1 count=20
```

### Test Program

```c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    int fd;
    char write_buf[] = "Test data from user space";
    char read_buf[100];
    ssize_t bytes;

    fd = open("/dev/simple_char", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* Write data */
    bytes = write(fd, write_buf, strlen(write_buf));
    printf("Wrote %zd bytes\n", bytes);

    /* Seek to beginning */
    lseek(fd, 0, SEEK_SET);

    /* Read data */
    bytes = read(fd, read_buf, sizeof(read_buf) - 1);
    if (bytes > 0) {
        read_buf[bytes] = '\0';
        printf("Read %zd bytes: %s\n", bytes, read_buf);
    }

    close(fd);
    return 0;
}
```

### Unload Module

```bash
sudo rmmod simple_char
dmesg | tail -1
# simple_char: unregistered
```

## Understanding the Code

### Device Structure

```c
struct simple_device {
    struct cdev cdev;       /* Character device structure */
    char buffer[4096];      /* Data buffer */
    size_t size;            /* Current data size */
    struct mutex lock;      /* Synchronization */
};
```

### Registration Sequence

1. `alloc_chrdev_region()` - Get device numbers
2. `cdev_init()` - Initialize cdev with file_operations
3. `cdev_add()` - Add cdev to kernel
4. `class_create()` - Create sysfs class
5. `device_create()` - Create device (triggers udev)

### Cleanup Sequence (Reverse Order)

1. `device_destroy()` - Remove device
2. `class_destroy()` - Remove class
3. `cdev_del()` - Remove cdev
4. `unregister_chrdev_region()` - Release device numbers

## Key Concepts

- **container_of**: Getting device structure from embedded cdev
- **private_data**: Storing device pointer for use in all operations
- **Mutex protection**: Preventing race conditions
- **EOF handling**: Returning 0 when no more data
