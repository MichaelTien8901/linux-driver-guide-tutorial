# PWM Controller Demo Driver

This example demonstrates how to implement a PWM provider (pwm_chip) driver.

## Features

- Virtual PWM controller with 4 channels
- Full pwm_ops implementation (apply, get_state)
- Debugfs interface for status monitoring
- Self-registering for easy testing

## Building

```bash
make
```

## Testing

```bash
# Load the module
sudo insmod pwm_demo.ko

# Find the PWM chip
ls /sys/class/pwm/

# Export a PWM channel (e.g., pwmchip0)
echo 0 | sudo tee /sys/class/pwm/pwmchip0/export

# Configure the PWM
# Set period to 1ms (1000000 ns)
echo 1000000 | sudo tee /sys/class/pwm/pwmchip0/pwm0/period

# Set duty cycle to 50% (500000 ns)
echo 500000 | sudo tee /sys/class/pwm/pwmchip0/pwm0/duty_cycle

# Enable the PWM
echo 1 | sudo tee /sys/class/pwm/pwmchip0/pwm0/enable

# Check status via debugfs
sudo cat /sys/kernel/debug/demo_pwm

# Unexport when done
echo 0 | sudo tee /sys/class/pwm/pwmchip0/unexport

# Unload the module
sudo rmmod pwm_demo
```

## sysfs Interface

```
/sys/class/pwm/pwmchipN/
├── npwm              # Number of PWM channels
├── export            # Export a channel
├── unexport          # Unexport a channel
└── pwmM/             # Exported channel
    ├── period        # Period in nanoseconds
    ├── duty_cycle    # Duty cycle in nanoseconds
    ├── polarity      # normal or inversed
    └── enable        # 0 or 1
```

## Key Implementation Details

### pwm_ops Callbacks

```c
static const struct pwm_ops demo_pwm_ops = {
    .request = demo_pwm_request,    /* Called when channel is exported */
    .free = demo_pwm_free,          /* Called when channel is unexported */
    .apply = demo_pwm_apply,        /* Apply new state */
    .get_state = demo_pwm_get_state,/* Read current state */
};
```

### State Application

```c
static int demo_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
                          const struct pwm_state *state)
{
    /* state contains: period, duty_cycle, polarity, enabled */
    /* Apply to hardware or virtual state */
    return 0;
}
```

## Files

- `pwm_demo.c` - Main driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- Part 10.1: PWM Subsystem
- Part 10.2: PWM Provider
