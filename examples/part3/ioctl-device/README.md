# IOCTL Device Example

A character device demonstrating IOCTL command handling.

## Files

- `ioctl_example.h` - Shared IOCTL definitions (kernel + user space)
- `ioctl_device.c` - Kernel module
- `test_ioctl.c` - User space test program
- `Makefile` - Build configuration

## IOCTL Commands

| Command | Type | Description |
|---------|------|-------------|
| `IOCTL_RESET` | `_IO` | Reset device to default state |
| `IOCTL_GET_STATS` | `_IOR` | Get device statistics |
| `IOCTL_GET_VALUE` | `_IOR` | Get single integer value |
| `IOCTL_SET_CONFIG` | `_IOW` | Set device configuration |
| `IOCTL_SET_VALUE` | `_IOW` | Set single integer value |
| `IOCTL_XFER_CONFIG` | `_IOWR` | Bidirectional config transfer |

## Building

```bash
# Build kernel module and test program
make

# Cross-compile kernel module for ARM64
make modules KERNEL_DIR=/path/to/kernel ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-

# Build test program for target (cross-compile separately if needed)
aarch64-linux-gnu-gcc -o test_ioctl test_ioctl.c
```

## Testing

### Load the Module

```bash
sudo insmod ioctl_device.ko
dmesg | tail -3
```

### Run Test Program

```bash
# Build and run tests
make test

# Or manually
gcc -o test_ioctl test_ioctl.c
sudo ./test_ioctl
```

### Expected Output

```
IOCTL Example Test Program
==========================

Device opened successfully

Test 1: RESET command
  RESET successful

Test 2: SET_CONFIG command
  Configuration set:
    Speed: 100
    Mode:  2
    Name:  test_device

Test 3: SET_VALUE command
  Value set to: 42

Test 4: GET_VALUE command
  Value read: 42

Test 5: GET_STATS command
Statistics:
  Reads:      0
  Writes:     0
  IOCTLs:     5
  Last error: 0

Test 6: XFER_CONFIG command (bidirectional)
  Received current configuration:
Configuration:
  Speed: 100
  Mode:  2
  Name:  test_device

Test 7: Invalid command (should fail)
  Expected error: Inappropriate ioctl for device

Test 8: Invalid configuration (should fail)
  Expected error: Invalid argument

Final Statistics:
Statistics:
  Reads:      0
  Writes:     0
  IOCTLs:     9
  Last error: -22

Device closed. All tests complete.
```

### Kernel Log

```bash
dmesg | grep ioctl_example
# ioctl_example: registered with major=243, minor=0
# ioctl_example: device opened
# ioctl_example: RESET command
# ioctl_example: SET_CONFIG command
# ioctl_example: config set: speed=100, mode=2, name=test_device
# ...
```

### Unload Module

```bash
sudo rmmod ioctl_device
```

## Key Concepts

### IOCTL Macros

```c
_IO(type, nr)        /* No data transfer */
_IOR(type, nr, type) /* Read from driver */
_IOW(type, nr, type) /* Write to driver */
_IOWR(type, nr, type) /* Both directions */
```

### Command Validation

```c
/* Check magic number */
if (_IOC_TYPE(cmd) != IOCTL_MAGIC)
    return -ENOTTY;

/* Check command number */
if (_IOC_NR(cmd) > IOCTL_MAXNR)
    return -ENOTTY;
```

### Data Transfer

```c
/* Read from user */
if (copy_from_user(&data, (void __user *)arg, sizeof(data)))
    return -EFAULT;

/* Write to user */
if (copy_to_user((void __user *)arg, &data, sizeof(data)))
    return -EFAULT;

/* Single values */
get_user(value, (int __user *)arg);
put_user(value, (int __user *)arg);
```

### Error Codes

| Code | Meaning |
|------|---------|
| `-ENOTTY` | Unknown command |
| `-EFAULT` | Bad user pointer |
| `-EINVAL` | Invalid argument |
| `-EBUSY` | Device busy |
| `-EPERM` | Permission denied |
