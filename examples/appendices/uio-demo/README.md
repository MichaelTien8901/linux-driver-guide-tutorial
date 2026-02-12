# UIO Demo

Demonstrates UIO (Userspace I/O) with a minimal kernel stub that exports memory to user space, and a user space program that accesses it via `mmap()`.

## Files

- `uio_demo.c` - Kernel module (UIO provider)
- `uio_userspace.c` - User space test program
- `Makefile` - Build configuration (kernel + userspace)
- `README.md` - This file

## Features Demonstrated

1. **Kernel side (UIO provider)**
   - `struct uio_info` configuration
   - Memory region export with `UIO_MEM_LOGICAL`
   - `devm_uio_register_device()` for managed registration

2. **User space side**
   - Opening `/dev/uioN`
   - `mmap()` to map device memory
   - Direct register-style read/write access

## Building

```bash
# Build both kernel module and user space program
make
```

## Testing

### Quick Test

```bash
make test
```

### Manual Testing

```bash
# Load kernel module
sudo insmod uio_demo.ko

# Check UIO device was created
ls -la /dev/uio*
cat /sys/class/uio/uio0/name

# Run user space test
sudo ./uio_userspace

# Or specify device path
sudo ./uio_userspace /dev/uio0

# Unload
sudo rmmod uio_demo
```

### Expected Output

```
UIO User Space Demo
====================

Opened /dev/uio0
Mapped 4096 bytes of device memory

Test 1: Read identification
  Device ID: 'UIO-DEMO'

Test 2: Write data
  Wrote: 'Hello from userspace!' at offset 64

Test 3: Read back
  Read:  'Hello from userspace!' at offset 64

Test 4: Structured register access
  Wrote: reg[32] = 0xDEADBEEF
  Wrote: reg[33] = 0x12345678
  Read:  reg[32] = 0xDEADBEEF
  Read:  reg[33] = 0x12345678

All tests passed.
```

## Key Takeaways

- UIO keeps the kernel stub minimal — just memory mapping and optional IRQ
- User space accesses device memory via `mmap()` on `/dev/uioN`
- Real UIO drivers use physical addresses from platform resources
- UIO is ideal for FPGA and custom hardware with simple register interfaces
