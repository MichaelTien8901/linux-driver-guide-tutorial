# RTC Demo Driver

This example demonstrates how to implement an RTC device driver with alarm support.

## Features

- Software-maintained time using kernel timekeeping
- Alarm support with timer-based notification
- Standard RTC sysfs interface
- hwclock compatibility

## Building

```bash
make
```

## Testing

```bash
# Load the module
sudo insmod rtc_demo.ko

# Find the RTC device
ls /sys/class/rtc/

# Read time using hwclock
sudo hwclock -r -f /dev/rtc1  # Use actual device number

# Read via sysfs
cat /sys/class/rtc/rtc1/date
cat /sys/class/rtc/rtc1/time

# Set time (requires root)
sudo hwclock --set --date="2024-01-15 10:30:00" -f /dev/rtc1

# Set alarm (5 seconds from now)
sudo sh -c 'echo +5 > /sys/class/rtc/rtc1/wakealarm'

# Read alarm
cat /sys/class/rtc/rtc1/wakealarm

# Run test script
make test

# Unload
sudo rmmod rtc_demo
```

## sysfs Interface

```
/sys/class/rtc/rtcN/
├── name                    # "demo-rtc"
├── date                    # Current date (YYYY-MM-DD)
├── time                    # Current time (HH:MM:SS)
├── since_epoch             # Seconds since Unix epoch
├── wakealarm               # Alarm time (epoch seconds)
├── max_user_freq           # Max interrupt frequency
└── hctosys                 # Hardware clock to system
```

## /dev Interface

```bash
# Read time programmatically
#include <linux/rtc.h>
#include <sys/ioctl.h>

int fd = open("/dev/rtc1", O_RDONLY);
struct rtc_time rtc_tm;
ioctl(fd, RTC_RD_TIME, &rtc_tm);

# Set alarm
struct rtc_wkalrm alarm = {
    .enabled = 1,
    .time = { ... }
};
ioctl(fd, RTC_ALM_SET, &alarm);
ioctl(fd, RTC_AIE_ON, 0);  # Enable alarm interrupt
```

## Key Implementation Details

### Time Tracking

```c
/* Software time based on kernel time */
static time64_t demo_rtc_get_time64(struct demo_rtc *drtc)
{
    ktime_t now = ktime_get_real();
    s64 elapsed_ns = ktime_to_ns(ktime_sub(now, drtc->base_ktime));
    return drtc->base_time + div_s64(elapsed_ns, NSEC_PER_SEC);
}
```

### Alarm Implementation

```c
/* Timer callback for alarm */
static void demo_rtc_alarm_callback(struct timer_list *t)
{
    struct demo_rtc *drtc = from_timer(drtc, t, alarm_timer);
    drtc->alarm_pending = true;
    rtc_update_irq(drtc->rtc, 1, RTC_AF | RTC_IRQF);
}

/* Set alarm */
static int demo_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
    time64_t alarm_time = rtc_tm_to_time64(&alrm->time);
    unsigned long delay_jiffies = (alarm_time - current_time) * HZ;
    mod_timer(&drtc->alarm_timer, jiffies + delay_jiffies);
}
```

### RTC Operations

```c
static const struct rtc_class_ops demo_rtc_ops = {
    .read_time = demo_rtc_read_time,
    .set_time = demo_rtc_set_time,
    .read_alarm = demo_rtc_read_alarm,
    .set_alarm = demo_rtc_set_alarm,
    .alarm_irq_enable = demo_rtc_alarm_irq_enable,
};
```

## Files

- `rtc_demo.c` - Main driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- Part 11.4: RTC Subsystem
- Part 11.5: RTC Driver
