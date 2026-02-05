# Module Parameters Example

Demonstrates various types of kernel module parameters.

## Parameters

| Parameter | Type | Permissions | Description |
|-----------|------|-------------|-------------|
| `count` | int | 0644 | Number of greetings |
| `name` | string | 0644 | Name to greet |
| `verbose` | bool | 0644 | Enable verbose output |
| `values` | int array | 0644 | Array of integers |
| `validated` | int | 0644 | Validated parameter (0-100) |

## Building

```bash
make KERNEL_DIR=/kernel/linux ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

## Testing

### Load with Default Parameters

```bash
sudo insmod params.ko
dmesg | tail
```

### Load with Custom Parameters

```bash
sudo insmod params.ko count=5 name="Alice" verbose=1
sudo insmod params.ko values=1,2,3,4,5 validated=75
```

### View Parameters at Runtime

```bash
cat /sys/module/params/parameters/count
cat /sys/module/params/parameters/name
cat /sys/module/params/parameters/verbose
cat /sys/module/params/parameters/validated
```

### Modify Parameters at Runtime

```bash
echo 10 > /sys/module/params/parameters/count
echo "Bob" > /sys/module/params/parameters/name
echo 1 > /sys/module/params/parameters/verbose

# This should fail (out of range)
echo 200 > /sys/module/params/parameters/validated
```

### View Module Information

```bash
modinfo params.ko
```

### Unload

```bash
sudo rmmod params
dmesg | tail
```

## Key Concepts Demonstrated

1. **module_param()** - Basic parameter declaration
2. **module_param_array()** - Array parameters
3. **module_param_cb()** - Parameters with custom get/set callbacks
4. **MODULE_PARM_DESC()** - Parameter documentation
5. **Sysfs permissions** - Making parameters readable/writable at runtime
