# Virtual Network Device Demo

A loopback-style network driver demonstrating core network driver concepts.

## What This Example Shows

- `net_device` allocation with `alloc_etherdev()`
- `net_device_ops` implementation (open, stop, start_xmit)
- NAPI for receive processing
- Statistics with `ndo_get_stats64`
- Queue control with `netif_start_queue()` / `netif_stop_queue()`

## How It Works

This driver creates a virtual network interface. Packets transmitted are looped back and received on the same interface (like a hardware loopback cable).

```
Application → TX → [Driver copies packet] → RX Ring → NAPI Poll → Network Stack
```

## Building

```bash
make
```

## Testing

```bash
# Load module
sudo insmod vnet_demo.ko

# Find the interface (will be named vnet0, eth1, or similar)
ip link show

# Configure IP address
sudo ip addr add 10.0.99.1/24 dev vnet0
sudo ip link set vnet0 up

# Check it's up
ip addr show vnet0

# View statistics
ip -s link show vnet0

# Generate traffic (loopback to self)
ping -c 5 10.0.99.1

# View updated statistics
ip -s link show vnet0

# Run automated test
make test

# Unload
sudo rmmod vnet_demo
```

## Key Code Sections

### Device Allocation

```c
vnet_dev = alloc_etherdev(sizeof(struct vnet_priv));
vnet_dev->netdev_ops = &vnet_netdev_ops;
eth_hw_addr_random(vnet_dev);
netif_napi_add(vnet_dev, &priv->napi, vnet_poll);
register_netdev(vnet_dev);
```

### Transmit Path

```c
static netdev_tx_t vnet_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    /* Update stats */
    priv->tx_packets++;
    priv->tx_bytes += skb->len;

    /* Loopback: queue copy for RX */
    rx_skb = skb_copy(skb, GFP_ATOMIC);
    rx_ring_enqueue(priv, rx_skb);
    napi_schedule(&priv->napi);  /* Trigger RX processing */

    dev_consume_skb_any(skb);    /* Free TX skb */
    return NETDEV_TX_OK;
}
```

### NAPI Poll

```c
static int vnet_poll(struct napi_struct *napi, int budget)
{
    int processed = 0;

    while (processed < budget && !rx_ring_empty(priv)) {
        skb = rx_ring_dequeue(priv);
        skb->protocol = eth_type_trans(skb, ndev);
        napi_gro_receive(napi, skb);
        processed++;
    }

    if (processed < budget)
        napi_complete_done(napi, processed);

    return processed;
}
```

## Files

- `vnet_demo.c` - Complete driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- [Part 12.1: Network Driver Concepts](../../../part12/01-concepts.md)
- [Part 12.2: Driver Skeleton](../../../part12/02-driver-skeleton.md)
- [Part 12.3: NAPI](../../../part12/03-napi.md)

## Further Reading

- [Network Drivers](https://docs.kernel.org/networking/netdevices.html)
- [NAPI](https://docs.kernel.org/networking/napi.html)
