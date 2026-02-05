# sysfs Attribute Demo

Demonstrates sysfs device attributes with DEVICE_ATTR macros.

## Testing

```bash
make
sudo insmod sysfs_demo.ko
cat /sys/devices/platform/sysfs_demo/status
echo 100 | sudo tee /sys/devices/platform/sysfs_demo/value
cat /sys/devices/platform/sysfs_demo/status
sudo rmmod sysfs_demo
```
