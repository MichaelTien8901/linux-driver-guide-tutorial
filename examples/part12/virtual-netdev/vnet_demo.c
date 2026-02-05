// SPDX-License-Identifier: GPL-2.0
/*
 * Virtual Network Device Demo
 *
 * A simple loopback-style network driver demonstrating:
 * - net_device allocation and registration
 * - Basic net_device_ops (open, stop, start_xmit)
 * - NAPI for RX processing
 * - Statistics tracking
 *
 * Packets transmitted are looped back and received on the same interface.
 */

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>

#define DRIVER_NAME "vnet_demo"
#define RX_RING_SIZE 64

struct vnet_priv {
    struct net_device *ndev;
    struct napi_struct napi;

    /* Simple RX ring buffer (simulates hardware) */
    struct sk_buff *rx_ring[RX_RING_SIZE];
    int rx_head;  /* Next slot to write */
    int rx_tail;  /* Next slot to read */
    spinlock_t rx_lock;

    /* Statistics */
    u64 tx_packets;
    u64 tx_bytes;
    u64 rx_packets;
    u64 rx_bytes;
};

/* ============ RX Ring Helpers (simulate hardware) ============ */

static bool rx_ring_full(struct vnet_priv *priv)
{
    return ((priv->rx_head + 1) % RX_RING_SIZE) == priv->rx_tail;
}

static bool rx_ring_empty(struct vnet_priv *priv)
{
    return priv->rx_head == priv->rx_tail;
}

static void rx_ring_enqueue(struct vnet_priv *priv, struct sk_buff *skb)
{
    priv->rx_ring[priv->rx_head] = skb;
    priv->rx_head = (priv->rx_head + 1) % RX_RING_SIZE;
}

static struct sk_buff *rx_ring_dequeue(struct vnet_priv *priv)
{
    struct sk_buff *skb = priv->rx_ring[priv->rx_tail];
    priv->rx_ring[priv->rx_tail] = NULL;
    priv->rx_tail = (priv->rx_tail + 1) % RX_RING_SIZE;
    return skb;
}

/* ============ NAPI Poll Function ============ */

static int vnet_poll(struct napi_struct *napi, int budget)
{
    struct vnet_priv *priv = container_of(napi, struct vnet_priv, napi);
    struct net_device *ndev = priv->ndev;
    int processed = 0;
    unsigned long flags;

    while (processed < budget) {
        struct sk_buff *skb;

        spin_lock_irqsave(&priv->rx_lock, flags);
        if (rx_ring_empty(priv)) {
            spin_unlock_irqrestore(&priv->rx_lock, flags);
            break;
        }
        skb = rx_ring_dequeue(priv);
        spin_unlock_irqrestore(&priv->rx_lock, flags);

        if (!skb)
            break;

        /* Set up skb for network stack */
        skb->protocol = eth_type_trans(skb, ndev);

        /* Update stats */
        priv->rx_packets++;
        priv->rx_bytes += skb->len;

        /* Hand to network stack */
        napi_gro_receive(napi, skb);
        processed++;
    }

    /* If we processed fewer than budget, we're done */
    if (processed < budget)
        napi_complete_done(napi, processed);

    return processed;
}

/* ============ Net Device Operations ============ */

static int vnet_open(struct net_device *ndev)
{
    struct vnet_priv *priv = netdev_priv(ndev);

    /* Enable NAPI */
    napi_enable(&priv->napi);

    /* Allow transmit */
    netif_start_queue(ndev);

    netdev_info(ndev, "Interface opened\n");
    return 0;
}

static int vnet_stop(struct net_device *ndev)
{
    struct vnet_priv *priv = netdev_priv(ndev);
    int i;

    /* Stop transmit */
    netif_stop_queue(ndev);

    /* Disable NAPI */
    napi_disable(&priv->napi);

    /* Clear RX ring */
    for (i = 0; i < RX_RING_SIZE; i++) {
        if (priv->rx_ring[i]) {
            dev_kfree_skb(priv->rx_ring[i]);
            priv->rx_ring[i] = NULL;
        }
    }
    priv->rx_head = priv->rx_tail = 0;

    netdev_info(ndev, "Interface stopped\n");
    return 0;
}

static netdev_tx_t vnet_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    struct vnet_priv *priv = netdev_priv(ndev);
    struct sk_buff *rx_skb;
    unsigned long flags;

    /* Update TX stats */
    priv->tx_packets++;
    priv->tx_bytes += skb->len;

    /*
     * Loopback: copy the packet and queue it for RX.
     * Real drivers would DMA to hardware here.
     */
    rx_skb = skb_copy(skb, GFP_ATOMIC);
    if (rx_skb) {
        spin_lock_irqsave(&priv->rx_lock, flags);
        if (!rx_ring_full(priv)) {
            rx_ring_enqueue(priv, rx_skb);
            /* Schedule NAPI to process it */
            napi_schedule(&priv->napi);
        } else {
            /* Ring full, drop packet */
            dev_kfree_skb(rx_skb);
            ndev->stats.rx_dropped++;
        }
        spin_unlock_irqrestore(&priv->rx_lock, flags);
    }

    /* Free original skb */
    dev_consume_skb_any(skb);

    return NETDEV_TX_OK;
}

static void vnet_get_stats64(struct net_device *ndev,
                             struct rtnl_link_stats64 *stats)
{
    struct vnet_priv *priv = netdev_priv(ndev);

    stats->tx_packets = priv->tx_packets;
    stats->tx_bytes = priv->tx_bytes;
    stats->rx_packets = priv->rx_packets;
    stats->rx_bytes = priv->rx_bytes;
    stats->rx_dropped = ndev->stats.rx_dropped;
}

static const struct net_device_ops vnet_netdev_ops = {
    .ndo_open       = vnet_open,
    .ndo_stop       = vnet_stop,
    .ndo_start_xmit = vnet_start_xmit,
    .ndo_get_stats64 = vnet_get_stats64,
    .ndo_set_mac_address = eth_mac_addr,
    .ndo_validate_addr = eth_validate_addr,
};

/* ============ Module Init/Exit ============ */

static struct net_device *vnet_dev;

static int __init vnet_init(void)
{
    struct vnet_priv *priv;
    int err;

    /* Allocate net_device with private data */
    vnet_dev = alloc_etherdev(sizeof(struct vnet_priv));
    if (!vnet_dev)
        return -ENOMEM;

    /* Set up private data */
    priv = netdev_priv(vnet_dev);
    priv->ndev = vnet_dev;
    spin_lock_init(&priv->rx_lock);

    /* Configure device */
    vnet_dev->netdev_ops = &vnet_netdev_ops;
    vnet_dev->needs_free_netdev = true;

    /* Generate random MAC address */
    eth_hw_addr_random(vnet_dev);

    /* Register NAPI */
    netif_napi_add(vnet_dev, &priv->napi, vnet_poll);

    /* Register with network stack */
    err = register_netdev(vnet_dev);
    if (err) {
        pr_err("Failed to register netdev: %d\n", err);
        free_netdev(vnet_dev);
        return err;
    }

    pr_info("Virtual network device '%s' registered (MAC: %pM)\n",
            vnet_dev->name, vnet_dev->dev_addr);
    return 0;
}

static void __exit vnet_exit(void)
{
    struct vnet_priv *priv = netdev_priv(vnet_dev);

    unregister_netdev(vnet_dev);
    netif_napi_del(&priv->napi);
    /* free_netdev handled by needs_free_netdev */

    pr_info("Virtual network device unregistered\n");
}

module_init(vnet_init);
module_exit(vnet_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Virtual Network Device Demo with NAPI");
