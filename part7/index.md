---
layout: default
title: "Part 7: Interrupt Handling"
nav_order: 8
has_children: true
---

# Part 7: Interrupt Handling

Interrupts allow hardware to signal the CPU that an event has occurred. Writing correct interrupt handlers is critical for responsive, stable drivers.

## Interrupt Flow Overview

```mermaid
flowchart TB
    subgraph Hardware
        DEV[Device]
        PIC[Interrupt Controller]
        CPU[CPU]
    end

    subgraph Kernel["Linux Kernel"]
        IDT[Interrupt Descriptor Table]
        GIH[Generic IRQ Handler]
        DRV[Your Driver Handler]
    end

    subgraph Response["Response"]
        IMM[Immediate Work]
        DEF[Deferred Work]
    end

    DEV -->|"Assert IRQ line"| PIC
    PIC -->|"Signal CPU"| CPU
    CPU -->|"Lookup handler"| IDT
    IDT --> GIH
    GIH -->|"Call registered handler"| DRV
    DRV --> IMM
    DRV -->|"Schedule"| DEF

    style Hardware fill:#73828d,stroke:#2196f3
    style Kernel fill:#8f8370,stroke:#ff9800
    style Response fill:#B37585,stroke:#9c27b0
```

## Chapter Contents

| Chapter | Topic | Key Concepts |
|---------|-------|--------------|
| [7.1]({% link part7/01-interrupt-concepts.md %}) | Interrupt Concepts | Hardware interrupts, IRQ numbers, contexts |
| [7.2]({% link part7/02-requesting-irqs.md %}) | Requesting IRQs | request_irq, devm_request_irq, free_irq |
| [7.3]({% link part7/03-interrupt-handlers.md %}) | Interrupt Handlers | Handler structure, return values, context |
| [7.4]({% link part7/04-top-bottom-halves.md %}) | Top and Bottom Halves | Splitting work, deferred execution |
| [7.5]({% link part7/05-tasklets.md %}) | Tasklets | tasklet_struct, scheduling, usage |
| [7.6]({% link part7/06-threaded-irqs.md %}) | Threaded IRQs | request_threaded_irq, hard/thread handlers |
| [7.7]({% link part7/07-shared-interrupts.md %}) | Shared Interrupts | IRQF_SHARED, checking ownership |
| [7.8]({% link part7/08-interrupt-control.md %}) | Interrupt Control | Enable/disable, IRQ affinity, edge vs level |

## Key Concepts

### The Interrupt Handling Model

```mermaid
sequenceDiagram
    participant HW as Hardware
    participant CPU as CPU
    participant TOP as Top Half (hardirq)
    participant BOT as Bottom Half

    HW->>CPU: Interrupt signal
    CPU->>CPU: Save context
    CPU->>TOP: Call handler
    Note over TOP: Runs with IRQs disabled
    TOP->>TOP: Acknowledge interrupt
    TOP->>TOP: Read/write urgent data
    TOP->>BOT: Schedule deferred work
    TOP->>CPU: Return IRQ_HANDLED
    CPU->>CPU: Restore context

    Note over BOT: Runs later with IRQs enabled
    BOT->>BOT: Process data
    BOT->>BOT: Wake waiters
```

### Why Split Work?

Interrupt handlers run with interrupts disabled (or at least their own IRQ masked). Long handlers:

- **Increase latency** for other interrupts
- **Block preemption** on that CPU
- **Can cause system instability** if they take too long

The solution: **Do minimal work in the handler, defer the rest.**

### Bottom Half Mechanisms

```mermaid
flowchart TB
    IRQ[Interrupt Handler]

    subgraph Mechanisms["Deferred Work Options"]
        TL[Tasklet]
        WQ[Workqueue]
        TH[Threaded Handler]
        ST[Softirq]
    end

    IRQ -->|"Atomic context"| TL
    IRQ -->|"Can sleep"| WQ
    IRQ -->|"Simple case"| TH
    IRQ -->|"High perf"| ST

    TL -->|"Softirq context"| Note1["Fast, can't sleep"]
    WQ -->|"Process context"| Note2["Can sleep, mutex ok"]
    TH -->|"Kernel thread"| Note3["Preferred for drivers"]
    ST -->|"Direct softirq"| Note4["Reserved for core kernel"]

    style IRQ fill:#c62828,stroke:#c62828
    style TL fill:#f9a825,stroke:#f9a825
    style WQ fill:#2e7d32,stroke:#2e7d32
    style TH fill:#1565c0,stroke:#1565c0
    style ST fill:#6a1b9a,stroke:#6a1b9a
```

## Interrupt Handler Rules

### What You CAN Do

- Read/write device registers
- Acknowledge the interrupt
- Schedule deferred work
- Wake up waiting processes
- Update statistics (atomic operations)

### What You CANNOT Do

- Sleep or call functions that might sleep
- Allocate memory with GFP_KERNEL
- Acquire mutexes
- Call copy_to_user/copy_from_user
- Take a long time (keep it fast!)

## Common Patterns

### Simple Handler (No Deferred Work)

```c
static irqreturn_t simple_handler(int irq, void *dev_id)
{
    struct my_device *dev = dev_id;

    /* Read status and acknowledge */
    u32 status = readl(dev->regs + STATUS);
    writel(status, dev->regs + STATUS);  /* Clear interrupt */

    return IRQ_HANDLED;
}
```

### Handler with Threaded Work

```c
/* Fast top half */
static irqreturn_t my_hardirq(int irq, void *dev_id)
{
    struct my_device *dev = dev_id;

    if (!device_generated_irq(dev))
        return IRQ_NONE;

    /* Acknowledge hardware */
    writel(0, dev->regs + IRQ_ACK);

    return IRQ_WAKE_THREAD;  /* Run threaded handler */
}

/* Threaded bottom half - can sleep */
static irqreturn_t my_thread(int irq, void *dev_id)
{
    struct my_device *dev = dev_id;

    /* Process data, can sleep here */
    process_data(dev);
    wake_up_interruptible(&dev->waitq);

    return IRQ_HANDLED;
}
```

## Examples

This part includes working examples:

- **irq-handler**: Complete interrupt handling with threaded IRQ

## Prerequisites

Before starting this part, ensure you understand:

- Device model and platform drivers (Part 6)
- Concurrency concepts (Part 4)
- Memory management (Part 5)

## Further Reading

- [IRQ Domain Documentation](https://docs.kernel.org/core-api/irq/index.html) - IRQ subsystem guide
- [Generic IRQ](https://docs.kernel.org/core-api/genericirq.html) - Generic IRQ handling

## Next

Start with [Interrupt Concepts]({% link part7/01-interrupt-concepts.md %}) to understand how hardware interrupts work.
