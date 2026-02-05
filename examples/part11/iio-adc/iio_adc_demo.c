// SPDX-License-Identifier: GPL-2.0
/*
 * IIO ADC Demo Driver
 *
 * Demonstrates implementing an IIO device for a virtual ADC.
 * Creates /sys/bus/iio/devices/iio:deviceN with voltage channels.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/random.h>

#define DRIVER_NAME     "demo-iio-adc"
#define NUM_CHANNELS    4
#define ADC_RESOLUTION  12          /* 12-bit ADC */
#define VREF_MV         3300        /* 3.3V reference */

struct demo_adc {
    struct mutex lock;
    int scale_mv;
    /* Simulated channel values (raw ADC counts) */
    int channels[NUM_CHANNELS];
};

/* Simulate reading an ADC channel with some variation */
static int demo_adc_read_channel(struct demo_adc *adc, int channel)
{
    int base = adc->channels[channel];
    int variation;

    get_random_bytes(&variation, sizeof(variation));
    variation = (variation % 100) - 50;  /* ±50 counts variation */

    return clamp(base + variation, 0, (1 << ADC_RESOLUTION) - 1);
}

static int demo_adc_read_raw(struct iio_dev *indio_dev,
                             struct iio_chan_spec const *chan,
                             int *val, int *val2, long mask)
{
    struct demo_adc *adc = iio_priv(indio_dev);
    int ret = 0;

    mutex_lock(&adc->lock);

    switch (mask) {
    case IIO_CHAN_INFO_RAW:
        *val = demo_adc_read_channel(adc, chan->channel);
        ret = IIO_VAL_INT;
        break;

    case IIO_CHAN_INFO_SCALE:
        /* Scale to convert raw to millivolts: Vref / 2^bits */
        *val = adc->scale_mv;
        *val2 = ADC_RESOLUTION;
        ret = IIO_VAL_FRACTIONAL_LOG2;
        break;

    case IIO_CHAN_INFO_OFFSET:
        *val = 0;
        ret = IIO_VAL_INT;
        break;

    default:
        ret = -EINVAL;
    }

    mutex_unlock(&adc->lock);
    return ret;
}

static int demo_adc_write_raw(struct iio_dev *indio_dev,
                              struct iio_chan_spec const *chan,
                              int val, int val2, long mask)
{
    struct demo_adc *adc = iio_priv(indio_dev);

    if (mask != IIO_CHAN_INFO_RAW)
        return -EINVAL;

    /* Allow setting simulated value for testing */
    if (val < 0 || val >= (1 << ADC_RESOLUTION))
        return -EINVAL;

    mutex_lock(&adc->lock);
    adc->channels[chan->channel] = val;
    mutex_unlock(&adc->lock);

    return 0;
}

#define DEMO_ADC_CHANNEL(num) {                                 \
    .type = IIO_VOLTAGE,                                        \
    .indexed = 1,                                               \
    .channel = (num),                                           \
    .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),              \
    .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) |     \
                                BIT(IIO_CHAN_INFO_OFFSET),      \
    .scan_index = (num),                                        \
    .scan_type = {                                              \
        .sign = 'u',                                            \
        .realbits = ADC_RESOLUTION,                             \
        .storagebits = 16,                                      \
        .endianness = IIO_CPU,                                  \
    },                                                          \
}

static const struct iio_chan_spec demo_adc_channels[] = {
    DEMO_ADC_CHANNEL(0),
    DEMO_ADC_CHANNEL(1),
    DEMO_ADC_CHANNEL(2),
    DEMO_ADC_CHANNEL(3),
};

static const struct iio_info demo_adc_info = {
    .read_raw = demo_adc_read_raw,
    .write_raw = demo_adc_write_raw,
};

static int demo_adc_probe(struct platform_device *pdev)
{
    struct demo_adc *adc;
    struct iio_dev *indio_dev;
    int i;

    indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*adc));
    if (!indio_dev)
        return -ENOMEM;

    adc = iio_priv(indio_dev);
    mutex_init(&adc->lock);
    adc->scale_mv = VREF_MV;

    /* Initialize with simulated values */
    adc->channels[0] = 2048;  /* ~1.65V (mid-scale) */
    adc->channels[1] = 4095;  /* ~3.3V (full-scale) */
    adc->channels[2] = 1024;  /* ~0.825V */
    adc->channels[3] = 0;     /* 0V */

    indio_dev->name = DRIVER_NAME;
    indio_dev->info = &demo_adc_info;
    indio_dev->modes = INDIO_DIRECT_MODE;
    indio_dev->channels = demo_adc_channels;
    indio_dev->num_channels = ARRAY_SIZE(demo_adc_channels);

    platform_set_drvdata(pdev, indio_dev);

    return devm_iio_device_register(&pdev->dev, indio_dev);
}

static const struct of_device_id demo_adc_of_match[] = {
    { .compatible = "demo,iio-adc" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_adc_of_match);

static struct platform_driver demo_adc_driver = {
    .probe = demo_adc_probe,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = demo_adc_of_match,
    },
};

/* Self-registering platform device */
static struct platform_device *demo_pdev;

static int __init demo_adc_init(void)
{
    int ret;

    ret = platform_driver_register(&demo_adc_driver);
    if (ret)
        return ret;

    demo_pdev = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
    if (IS_ERR(demo_pdev)) {
        platform_driver_unregister(&demo_adc_driver);
        return PTR_ERR(demo_pdev);
    }

    pr_info("Demo IIO ADC registered\n");
    return 0;
}

static void __exit demo_adc_exit(void)
{
    platform_device_unregister(demo_pdev);
    platform_driver_unregister(&demo_adc_driver);
}

module_init(demo_adc_init);
module_exit(demo_adc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Demo IIO ADC Driver");
