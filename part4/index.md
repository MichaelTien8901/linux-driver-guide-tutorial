---
layout: default
title: "Part 4: Concurrency and Synchronization"
nav_order: 5
has_children: true
---

# Part 4: Concurrency and Synchronization

The Linux kernel is inherently concurrent - multiple CPUs, interrupts, and preemption mean your code can be interrupted or run simultaneously at any time. Understanding synchronization primitives is essential for writing correct, safe drivers.

## Why Concurrency Matters

```mermaid
flowchart TD
    subgraph Sources["Concurrency Sources"]
        direction TB
        SMP["Multiple CPUs"]
        INT["Interrupts"]
        PRE["Preemption"]
        SOFT["Softirqs/Tasklets"]
    end

    subgraph Problem["The Problem"]
        direction TB
        Race["Race Conditions"]
        Corrupt["Data Corruption"]
        Dead["Deadlocks"]
    end

    subgraph Solutions["Solutions"]
        direction TB
        Spin["Spinlocks"]
        Mutex["Mutexes"]
        Atomic["Atomic Operations"]
        RCU["RCU"]
    end

    Sources --> Problem
    Problem --> Solutions

    style Problem fill:#8f6d72,stroke:#c62828
    style Solutions fill:#7a8f73,stroke:#2e7d32
```

## Chapter Contents

| Chapter | Topic | Key Concepts |
|---------|-------|--------------|
| [4.1]({% link part4/01-concurrency-concepts.md %}) | Concurrency Concepts | Race conditions, critical sections |
| [4.2]({% link part4/02-spinlocks.md %}) | Spinlocks | spin_lock, spin_lock_irqsave |
| [4.3]({% link part4/03-mutexes.md %}) | Mutexes | Sleeping locks, mutex vs spinlock |
| [4.4]({% link part4/04-semaphores.md %}) | Semaphores | Counting semaphores |
| [4.5]({% link part4/05-atomic-operations.md %}) | Atomic Operations | atomic_t, bitwise atomics |
| [4.6]({% link part4/06-rcu.md %}) | RCU | Read-Copy-Update |
| [4.7]({% link part4/07-completions.md %}) | Completions | Signaling between threads |
| [4.8]({% link part4/08-workqueues.md %}) | Work Queues | Deferred processing |
| [4.9]({% link part4/09-lockdep.md %}) | Lockdep | Deadlock detection |
| [4.10]({% link part4/10-timers.md %}) | Kernel Timers | timer_list, hrtimer |
| [4.11]({% link part4/11-wait-queues.md %}) | Wait Queues | wait_event, wake_up |
| [4.12]({% link part4/12-kthreads.md %}) | Kernel Threads | kthread_run, kthread_stop |

## Quick Reference: Choosing a Lock

```mermaid
flowchart TD
    Start["Need to protect data?"]
    Sleep{"Can you sleep?"}
    Duration{"Short critical section?"}
    Read{"Mostly reads?"}
    Counter{"Simple counter?"}

    Start --> Sleep
    Sleep -->|"No (interrupt context)"| Spin["Spinlock"]
    Sleep -->|"Yes (process context)"| Duration
    Duration -->|"Yes"| Spin
    Duration -->|"No"| Mutex["Mutex"]
    Sleep -->|"Yes"| Read
    Read -->|"Yes, rare writes"| RCU["RCU"]
    Sleep -->|"Yes"| Counter
    Counter -->|"Yes"| Atomic["atomic_t"]

    style Spin fill:#8f8a73,stroke:#f9a825
    style Mutex fill:#738f99,stroke:#0277bd
    style RCU fill:#8f7392,stroke:#6a1b9a
    style Atomic fill:#7a8f73,stroke:#2e7d32
```

| Primitive | Sleep OK? | Interrupt Safe? | Best For |
|-----------|-----------|-----------------|----------|
| Spinlock | No | With irqsave | Short critical sections |
| Mutex | Yes | No | Longer critical sections |
| Semaphore | Yes | No | Resource counting |
| atomic_t | N/A | Yes | Simple counters |
| RCU | Read: Yes | Read: Yes | Read-heavy data |
| Completion | Yes | No | Thread synchronization |

## Examples

This part includes working examples:

- **spinlock-demo**: Spinlock usage patterns
- **workqueue-demo**: Deferred work processing
- **timer-demo**: Kernel timer and hrtimer usage
- **kthread-demo**: Background kernel threads

## Common Pitfalls

{: .warning }
**Deadlocks**: Taking locks in different orders across code paths

{: .warning }
**Priority inversion**: Low-priority task holds lock needed by high-priority task

{: .warning }
**Sleeping with spinlock**: Calling functions that might sleep while holding a spinlock

## Prerequisites

Before starting this part, ensure you understand:

- Process vs interrupt context (Part 2)
- Basic module structure (Part 1-2)
- Character device operations (Part 3)

## Further Reading

- [Locking Documentation](https://docs.kernel.org/locking/index.html) - Kernel locking guide
- [RCU Documentation](https://docs.kernel.org/RCU/index.html) - RCU concepts and API
- [Workqueue Documentation](https://docs.kernel.org/core-api/workqueue.html) - Work queue API
- [Timer Documentation](https://docs.kernel.org/timers/index.html) - Kernel timers
- [Kthread Documentation](https://docs.kernel.org/driver-api/basics.html#kernel-threads) - Kernel threads

## Next

Start with [Concurrency Concepts]({% link part4/01-concurrency-concepts.md %}) to understand why synchronization is necessary.
