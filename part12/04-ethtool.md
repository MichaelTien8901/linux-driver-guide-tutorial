---
layout: default
title: "12.4 Ethtool Operations"
parent: "Part 12: Network Device Drivers"
nav_order: 4
---

# Ethtool Operations

Ethtool provides a standard interface for configuring network devices. Users run `ethtool eth0` to query settings, and your driver responds via `ethtool_ops`.

## Why Ethtool?

```bash
# Common ethtool commands your driver can support
ethtool eth0              # Show link settings (speed, duplex)
ethtool -i eth0           # Show driver info
ethtool -S eth0           # Show detailed statistics
ethtool -s eth0 speed 100 # Set link speed
ethtool -k eth0           # Show offload features
```

Without `ethtool_ops`, these commands return minimal or no information.

## Basic Ethtool Setup

```c
#include <linux/ethtool.h>

static const struct ethtool_ops my_ethtool_ops = {
    .get_drvinfo     = my_get_drvinfo,
    .get_link        = ethtool_op_get_link,  /* Use kernel helper */
    .get_link_ksettings = my_get_link_ksettings,
    .set_link_ksettings = my_set_link_ksettings,
};

/* In probe or init */
ndev->ethtool_ops = &my_ethtool_ops;
```

## Driver Information

The most basic operation - identify your driver:

```c
static void my_get_drvinfo(struct net_device *ndev,
                           struct ethtool_drvinfo *info)
{
    struct my_priv *priv = netdev_priv(ndev);

    strscpy(info->driver, "my_netdrv", sizeof(info->driver));
    strscpy(info->version, "1.0", sizeof(info->version));
    strscpy(info->bus_info, dev_name(ndev->dev.parent),
            sizeof(info->bus_info));
}
```

Result of `ethtool -i eth0`:
```
driver: my_netdrv
version: 1.0
bus_info: platform:10000000.ethernet
```

## Link Settings

Report and configure speed, duplex, and autonegotiation:

```c
static int my_get_link_ksettings(struct net_device *ndev,
                                  struct ethtool_link_ksettings *cmd)
{
    struct my_priv *priv = netdev_priv(ndev);

    /* Report supported modes */
    ethtool_link_ksettings_zero_link_mode(cmd, supported);
    ethtool_link_ksettings_add_link_mode(cmd, supported, 10baseT_Half);
    ethtool_link_ksettings_add_link_mode(cmd, supported, 10baseT_Full);
    ethtool_link_ksettings_add_link_mode(cmd, supported, 100baseT_Half);
    ethtool_link_ksettings_add_link_mode(cmd, supported, 100baseT_Full);
    ethtool_link_ksettings_add_link_mode(cmd, supported, 1000baseT_Full);
    ethtool_link_ksettings_add_link_mode(cmd, supported, Autoneg);

    /* Copy supported to advertising */
    linkmode_copy(cmd->link_modes.advertising, cmd->link_modes.supported);

    /* Report current settings */
    cmd->base.speed = priv->link_speed;      /* SPEED_100, SPEED_1000, etc. */
    cmd->base.duplex = priv->duplex;         /* DUPLEX_FULL or DUPLEX_HALF */
    cmd->base.autoneg = priv->autoneg;       /* AUTONEG_ENABLE or DISABLE */
    cmd->base.port = PORT_MII;

    return 0;
}

static int my_set_link_ksettings(struct net_device *ndev,
                                  const struct ethtool_link_ksettings *cmd)
{
    struct my_priv *priv = netdev_priv(ndev);

    /* Validate and apply settings */
    if (cmd->base.autoneg == AUTONEG_ENABLE) {
        priv->autoneg = AUTONEG_ENABLE;
        /* Trigger autonegotiation */
    } else {
        /* Validate speed/duplex combination */
        if (cmd->base.speed == SPEED_1000 &&
            cmd->base.duplex == DUPLEX_HALF)
            return -EINVAL;  /* 1000 half not supported */

        priv->autoneg = AUTONEG_DISABLE;
        priv->link_speed = cmd->base.speed;
        priv->duplex = cmd->base.duplex;
        /* Apply to hardware */
    }

    return 0;
}
```

## Link Status

Use the kernel helper or implement your own:

```c
/* Simple: use carrier status */
.get_link = ethtool_op_get_link,

/* Or custom implementation */
static u32 my_get_link(struct net_device *ndev)
{
    struct my_priv *priv = netdev_priv(ndev);

    /* Read hardware link status register */
    return !!(readl(priv->regs + LINK_STATUS) & LINK_UP);
}
```

## Extended Statistics

Provide detailed counters beyond the basic stats:

```c
static const char my_stats_strings[][ETH_GSTRING_LEN] = {
    "tx_packets",
    "rx_packets",
    "tx_bytes",
    "rx_bytes",
    "tx_errors",
    "rx_errors",
    "rx_crc_errors",
    "rx_frame_errors",
    "tx_fifo_errors",
    "rx_fifo_errors",
    "tx_collisions",
};

#define MY_STATS_COUNT ARRAY_SIZE(my_stats_strings)

static void my_get_strings(struct net_device *ndev, u32 stringset, u8 *data)
{
    if (stringset == ETH_SS_STATS)
        memcpy(data, my_stats_strings, sizeof(my_stats_strings));
}

static int my_get_sset_count(struct net_device *ndev, int sset)
{
    if (sset == ETH_SS_STATS)
        return MY_STATS_COUNT;
    return -EOPNOTSUPP;
}

static void my_get_ethtool_stats(struct net_device *ndev,
                                  struct ethtool_stats *stats, u64 *data)
{
    struct my_priv *priv = netdev_priv(ndev);
    int i = 0;

    data[i++] = priv->stats.tx_packets;
    data[i++] = priv->stats.rx_packets;
    data[i++] = priv->stats.tx_bytes;
    data[i++] = priv->stats.rx_bytes;
    data[i++] = priv->stats.tx_errors;
    data[i++] = priv->stats.rx_errors;
    data[i++] = priv->hw_stats.rx_crc_errors;
    data[i++] = priv->hw_stats.rx_frame_errors;
    data[i++] = priv->hw_stats.tx_fifo_errors;
    data[i++] = priv->hw_stats.rx_fifo_errors;
    data[i++] = priv->hw_stats.tx_collisions;
}

static const struct ethtool_ops my_ethtool_ops = {
    .get_drvinfo     = my_get_drvinfo,
    .get_link        = ethtool_op_get_link,
    .get_strings     = my_get_strings,
    .get_sset_count  = my_get_sset_count,
    .get_ethtool_stats = my_get_ethtool_stats,
    /* ... */
};
```

Result of `ethtool -S eth0`:
```
NIC statistics:
     tx_packets: 12345
     rx_packets: 67890
     tx_bytes: 1234567
     ...
```

## Offload Features

Report and control hardware offload capabilities:

```c
static int my_get_ts_info(struct net_device *ndev,
                          struct ethtool_ts_info *info)
{
    /* Report timestamping capabilities if supported */
    info->so_timestamping = SOF_TIMESTAMPING_TX_SOFTWARE |
                            SOF_TIMESTAMPING_RX_SOFTWARE;
    info->phc_index = -1;  /* No PTP hardware clock */
    return 0;
}

/* Features are also controlled via netdev_features */
static netdev_features_t my_fix_features(struct net_device *ndev,
                                          netdev_features_t features)
{
    /* Disable features that can't work together */
    if (features & NETIF_F_HW_CSUM)
        features &= ~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);

    return features;
}

static int my_set_features(struct net_device *ndev,
                           netdev_features_t features)
{
    struct my_priv *priv = netdev_priv(ndev);

    /* Apply feature changes to hardware */
    if (features & NETIF_F_RXCSUM)
        hw_enable_rx_csum(priv);
    else
        hw_disable_rx_csum(priv);

    return 0;
}
```

## Complete Ethtool Operations

```c
static const struct ethtool_ops my_ethtool_ops = {
    /* Basic info */
    .get_drvinfo         = my_get_drvinfo,

    /* Link status and settings */
    .get_link            = ethtool_op_get_link,
    .get_link_ksettings  = my_get_link_ksettings,
    .set_link_ksettings  = my_set_link_ksettings,

    /* Statistics */
    .get_strings         = my_get_strings,
    .get_sset_count      = my_get_sset_count,
    .get_ethtool_stats   = my_get_ethtool_stats,

    /* Optional: message level */
    .get_msglevel        = my_get_msglevel,
    .set_msglevel        = my_set_msglevel,

    /* Optional: ring buffer size */
    .get_ringparam       = my_get_ringparam,
    .set_ringparam       = my_set_ringparam,
};
```

## Summary

| Operation | User Command | Callback |
|-----------|--------------|----------|
| Driver info | `ethtool -i` | `get_drvinfo` |
| Link status | `ethtool` | `get_link` |
| Speed/duplex | `ethtool` | `get_link_ksettings` |
| Statistics | `ethtool -S` | `get_ethtool_stats` |
| Features | `ethtool -k` | Handled via `netdev_features` |

{: .tip }
> Start with `get_drvinfo` and `get_link`. Add more operations as needed. Use kernel helpers (like `ethtool_op_get_link`) when available.

## Further Reading

- [Ethtool Documentation](https://docs.kernel.org/networking/ethtool-netlink.html) - Modern netlink API
- [ethtool_ops](https://elixir.bootlin.com/linux/v6.6/source/include/linux/ethtool.h) - All available operations
