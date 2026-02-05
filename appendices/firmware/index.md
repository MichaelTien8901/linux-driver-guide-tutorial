---
layout: default
title: "Appendix C: Firmware Loading"
nav_order: 19
---

# Appendix C: Firmware Loading

Loading firmware blobs for hardware initialization.

## Basic Pattern

```c
#include <linux/firmware.h>

static int my_probe(struct platform_device *pdev)
{
    const struct firmware *fw;
    int ret;

    /* Request firmware - blocks until loaded */
    ret = request_firmware(&fw, "my_device.fw", &pdev->dev);
    if (ret) {
        dev_err(&pdev->dev, "Firmware not found\n");
        return ret;
    }

    /* Use firmware */
    dev_info(&pdev->dev, "Firmware loaded: %zu bytes\n", fw->size);
    upload_to_device(fw->data, fw->size);

    /* Release when done */
    release_firmware(fw);

    return 0;
}
```

## Firmware Location

The kernel searches these paths (in order):
1. `/lib/firmware/updates/$(uname -r)/`
2. `/lib/firmware/updates/`
3. `/lib/firmware/$(uname -r)/`
4. `/lib/firmware/`

## Async Loading

For non-blocking firmware load:

```c
static void fw_callback(const struct firmware *fw, void *context)
{
    struct my_dev *dev = context;

    if (!fw) {
        dev_err(dev->dev, "Firmware load failed\n");
        return;
    }

    upload_to_device(fw->data, fw->size);
    release_firmware(fw);
}

static int my_probe(struct platform_device *pdev)
{
    /* Returns immediately, callback called when ready */
    return request_firmware_nowait(THIS_MODULE, true, "my_device.fw",
                                   &pdev->dev, GFP_KERNEL, dev, fw_callback);
}
```

## Direct Filesystem Access

For fallback or optional firmware:

```c
ret = firmware_request_nowarn(&fw, "optional.fw", dev);
if (ret) {
    dev_dbg(dev, "Optional firmware not found, using defaults\n");
    /* Continue without firmware */
}
```

## Summary

| Function | Behavior |
|----------|----------|
| `request_firmware()` | Blocking load |
| `request_firmware_nowait()` | Async with callback |
| `firmware_request_nowarn()` | No warning if missing |
| `release_firmware()` | Free firmware buffer |

## Further Reading

- [Firmware API](https://docs.kernel.org/driver-api/firmware/index.html)
- [Firmware Guidelines](https://docs.kernel.org/driver-api/firmware/fw_search_path.html)
