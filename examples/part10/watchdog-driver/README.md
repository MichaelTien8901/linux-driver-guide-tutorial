# Watchdog Timer Demo Driver

This example demonstrates how to implement a watchdog timer driver using the watchdog_device structure.

## Features

- Virtual watchdog with configurable timeout
- Timeout simulation (logs instead of resetting)
- Magic close support
- Nowayout mode support
- Time left reporting

## Building

```bash
make
```

## Testing

```bash
# Load the module
sudo insmod watchdog_demo.ko

# Check the watchdog device
ls -la /dev/watchdog*

# View watchdog info using wdctl
wdctl /dev/watchdog0

# Manual testing with simple write
# WARNING: This starts the watchdog!
sudo bash -c 'echo -n "start" > /dev/watchdog'

# Kick the watchdog periodically
while true; do
    sudo bash -c 'echo -n "k" > /dev/watchdog'
    sleep 10
done

# Magic close (allows closing without reset)
sudo bash -c 'echo -n "V" > /dev/watchdog'
```

## Using ioctl Interface

```c
#include <linux/watchdog.h>
#include <sys/ioctl.h>

int fd = open("/dev/watchdog", O_RDWR);

// Get info
struct watchdog_info info;
ioctl(fd, WDIOC_GETSUPPORT, &info);

// Set timeout
int timeout = 30;
ioctl(fd, WDIOC_SETTIMEOUT, &timeout);

// Get timeout
ioctl(fd, WDIOC_GETTIMEOUT, &timeout);

// Get time left
int timeleft;
ioctl(fd, WDIOC_GETTIMELEFT, &timeleft);

// Kick watchdog
int dummy;
ioctl(fd, WDIOC_KEEPALIVE, &dummy);

// Magic close before closing
write(fd, "V", 1);
close(fd);
```

## sysfs Interface

```bash
# View watchdog info
cat /sys/class/watchdog/watchdog0/identity
cat /sys/class/watchdog/watchdog0/timeout
cat /sys/class/watchdog/watchdog0/timeleft
cat /sys/class/watchdog/watchdog0/state

# Check status
cat /sys/class/watchdog/watchdog0/status
cat /sys/class/watchdog/watchdog0/bootstatus
```

## Module Parameters

- `nowayout`: Set to 1 to prevent watchdog from being stopped once started (default: depends on kernel config)

```bash
# Load with nowayout
sudo insmod watchdog_demo.ko nowayout=1
```

## Key Implementation Details

### Watchdog Operations

```c
static const struct watchdog_ops demo_wdt_ops = {
    .owner = THIS_MODULE,
    .start = demo_wdt_start,       /* Start watchdog */
    .stop = demo_wdt_stop,         /* Stop watchdog */
    .ping = demo_wdt_ping,         /* Kick/refresh watchdog */
    .set_timeout = demo_wdt_set_timeout,
    .get_timeleft = demo_wdt_get_timeleft,
};
```

### Magic Close

The driver supports "magic close" - writing 'V' before closing allows stopping the watchdog:
```c
WDIOF_MAGICCLOSE  /* in watchdog_info.options */
```

## Files

- `watchdog_demo.c` - Main driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- Part 10.3: Watchdog Subsystem
- Part 10.4: Watchdog Driver
