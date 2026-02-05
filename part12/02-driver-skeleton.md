---
layout: default
title: "12.2 Driver Skeleton"
parent: "Part 12: Network Device Drivers"
nav_order: 2
---

# Network Driver Skeleton

This chapter shows the typical structure of a network driver. Focus on the **pattern**, not memorizing every function.

## Minimal Driver Structure

```c
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

struct my_priv {
    struct net_device *ndev;
    /* Your hardware state, DMA descriptors, etc. */
};

/* ============ Net Device Operations ============ */

static int my_open(struct net_device *ndev)
{
    /* Called when: ip link set dev X up */
    netif_start_queue(ndev);  /* Allow TX */
    return 0;
}

static int my_stop(struct net_device *ndev)
{
    /* Called when: ip link set dev X down */
    netif_stop_queue(ndev);   /* Stop TX */
    return 0;
}

static netdev_tx_t my_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    /* Called for every packet to transmit */
    struct my_priv *priv = netdev_priv(ndev);

    /* TODO: Send skb->data (skb->len bytes) to hardware */

    dev_kfree_skb(skb);
    return NETDEV_TX_OK;
}

static const struct net_device_ops my_netdev_ops = {
    .ndo_open       = my_open,
    .ndo_stop       = my_stop,
    .ndo_start_xmit = my_start_xmit,
    /* Add more as needed - see below */
};

/* ============ Module Init/Exit ============ */

static int __init my_init(void)
{
    struct net_device *ndev;
    struct my_priv *priv;

    /* Allocate net_device + private data */
    ndev = alloc_etherdev(sizeof(struct my_priv));
    if (!ndev)
        return -ENOMEM;

    priv = netdev_priv(ndev);
    priv->ndev = ndev;

    /* Configure */
    ndev->netdev_ops = &my_netdev_ops;
    eth_hw_addr_random(ndev);  /* Generate random MAC */

    /* Register */
    if (register_netdev(ndev)) {
        free_netdev(ndev);
        return -ENODEV;
    }

    pr_info("Registered %s\n", ndev->name);
    return 0;
}

static void __exit my_exit(void)
{
    /* Cleanup: unregister then free */
    unregister_netdev(ndev);
    free_netdev(ndev);
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
```

## Essential Callbacks

You must implement at least these three:

| Callback | When Called | Your Job |
|----------|-------------|----------|
| `ndo_open` | `ip link set up` | Enable hardware, start queues |
| `ndo_stop` | `ip link set down` | Disable hardware, stop queues |
| `ndo_start_xmit` | Every TX packet | Send to hardware, free skb |

## Common Optional Callbacks

Add these as needed:

```c
static const struct net_device_ops my_netdev_ops = {
    /* Required */
    .ndo_open       = my_open,
    .ndo_stop       = my_stop,
    .ndo_start_xmit = my_start_xmit,

    /* Statistics */
    .ndo_get_stats64 = my_get_stats64,

    /* Configuration */
    .ndo_set_mac_address = eth_mac_addr,      /* Use default */
    .ndo_validate_addr   = eth_validate_addr, /* Use default */
    .ndo_change_mtu      = my_change_mtu,

    /* Multicast/promiscuous */
    .ndo_set_rx_mode = my_set_rx_mode,
};
```

{: .note }
> Don't implement callbacks you don't need. The kernel has sensible defaults. Start minimal, add as requirements grow.

## Allocation Helpers

| Function | Use For |
|----------|---------|
| `alloc_etherdev(priv_size)` | Standard Ethernet (MAC, MTU=1500) |
| `alloc_etherdev_mq(priv_size, queues)` | Multi-queue Ethernet |
| `alloc_netdev(priv_size, name, setup)` | Custom device types |

## Queue Control

Control whether the kernel can send you packets:

```c
netif_start_queue(ndev);  /* Allow TX - call in ndo_open */
netif_stop_queue(ndev);   /* Block TX - call when HW buffer full */
netif_wake_queue(ndev);   /* Resume TX - call when HW has space */
```

**Pattern for TX flow control:**

```c
static netdev_tx_t my_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    /* Queue packet to hardware */
    enqueue_to_hw(skb);

    /* If hardware buffer is full, stop kernel from sending more */
    if (hw_buffer_full())
        netif_stop_queue(ndev);

    return NETDEV_TX_OK;
}

/* In TX completion interrupt */
static void tx_complete_irq(struct my_priv *priv)
{
    /* Free transmitted skbs */

    /* If we stopped the queue and now have space, wake it */
    if (netif_queue_stopped(priv->ndev) && hw_has_space())
        netif_wake_queue(priv->ndev);
}
```

## Statistics

Implement `ndo_get_stats64` for accurate statistics:

```c
static void my_get_stats64(struct net_device *ndev,
                           struct rtnl_link_stats64 *stats)
{
    struct my_priv *priv = netdev_priv(ndev);

    stats->rx_packets = priv->rx_packets;
    stats->tx_packets = priv->tx_packets;
    stats->rx_bytes   = priv->rx_bytes;
    stats->tx_bytes   = priv->tx_bytes;
    stats->rx_errors  = priv->rx_errors;
    stats->tx_errors  = priv->tx_errors;
    /* ... more fields available */
}
```

{: .tip }
> Update your private counters in TX/RX paths, then copy them in `get_stats64`. This avoids locking in the hot path.

## Platform Driver Integration

Real drivers are typically platform or PCI drivers. Here's the pattern:

```c
static int my_probe(struct platform_device *pdev)
{
    struct net_device *ndev;
    struct my_priv *priv;

    ndev = devm_alloc_etherdev(&pdev->dev, sizeof(*priv));
    if (!ndev)
        return -ENOMEM;

    SET_NETDEV_DEV(ndev, &pdev->dev);  /* Link to platform device */
    priv = netdev_priv(ndev);
    platform_set_drvdata(pdev, ndev);

    /* Map registers, get IRQ from DT, etc. */

    ndev->netdev_ops = &my_netdev_ops;

    return register_netdev(ndev);
}

static void my_remove(struct platform_device *pdev)
{
    struct net_device *ndev = platform_get_drvdata(pdev);
    unregister_netdev(ndev);
    /* devm handles freeing */
}
```

## Summary

1. **Allocate**: `alloc_etherdev(sizeof(priv))`
2. **Configure**: Set `netdev_ops`, MAC address
3. **Register**: `register_netdev()`
4. **Implement**: At minimum `open`, `stop`, `start_xmit`
5. **Control queues**: `netif_start/stop/wake_queue()`

## Further Reading

- [net_device_ops](https://elixir.bootlin.com/linux/v6.6/source/include/linux/netdevice.h) - All available callbacks
- [Network Drivers](https://docs.kernel.org/networking/netdevices.html) - Registration details
