---
layout: default
title: "4.4 Semaphores"
parent: "Part 4: Concurrency and Synchronization"
nav_order: 4
---

# Semaphores

Semaphores are a more general form of mutex that can allow multiple tasks to hold the lock simultaneously. They're useful for resource counting and producer-consumer scenarios.

## Types of Semaphores

| Type | Count | Use Case |
|------|-------|----------|
| Binary | 0 or 1 | Mutual exclusion (use mutex instead) |
| Counting | 0 to N | Resource pool management |

{: .note }
For mutual exclusion, prefer mutexes over binary semaphores. Mutexes have better debugging support and clearer semantics.

## Basic Semaphore Usage

```c
#include <linux/semaphore.h>

/* Static initialization with count N */
static DEFINE_SEMAPHORE(my_sem);  /* Binary semaphore */

/* Dynamic initialization */
struct semaphore my_sem;
sema_init(&my_sem, count);  /* Initialize with count */

/* Acquire (decrement) - sleep if count is 0 */
down(&my_sem);
/* ... use the resource ... */
/* Release (increment) */
up(&my_sem);
```

## Semaphore Operations

### Basic Down/Up

```c
down(&sem);  /* Sleep until count > 0, then decrement */
/* ... */
up(&sem);    /* Increment count, wake up waiters */
```

### Interruptible Down

```c
if (down_interruptible(&sem)) {
    /* Interrupted by signal */
    return -ERESTARTSYS;
}
/* Got the semaphore */
up(&sem);
```

### Killable Down

```c
if (down_killable(&sem)) {
    /* Killed by fatal signal */
    return -ERESTARTSYS;
}
/* Got the semaphore */
up(&sem);
```

### Try Down (Non-blocking)

```c
if (down_trylock(&sem)) {
    /* Semaphore not available */
    return -EAGAIN;
}
/* Got the semaphore */
up(&sem);
```

### Timeout

```c
ret = down_timeout(&sem, msecs_to_jiffies(1000));
if (ret) {
    /* Timed out or interrupted */
    return ret;
}
/* Got the semaphore */
up(&sem);
```

## Counting Semaphore Example

Limit concurrent access to a resource pool:

```c
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/fs.h>

#define MAX_USERS 4

struct my_device {
    struct semaphore sem;
    int resources[MAX_USERS];
};

static struct my_device dev;

static int my_open(struct inode *inode, struct file *file)
{
    int ret;

    /* Try to acquire a slot */
    ret = down_interruptible(&dev.sem);
    if (ret)
        return ret;

    pr_info("Acquired slot (available: %d)\n",
            dev.sem.count);
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    up(&dev.sem);
    pr_info("Released slot (available: %d)\n",
            dev.sem.count);
    return 0;
}

static int __init my_init(void)
{
    /* Initialize with MAX_USERS slots */
    sema_init(&dev.sem, MAX_USERS);
    pr_info("Device allows %d concurrent users\n", MAX_USERS);
    return 0;
}
```

Testing:

```bash
# Open 4 times (will succeed)
for i in 1 2 3 4; do
    cat /dev/mydevice &
done

# 5th open will block
cat /dev/mydevice  # Waits until someone closes

# Close one
kill %1  # Now the 5th open proceeds
```

## Producer-Consumer Pattern

```c
#include <linux/semaphore.h>
#include <linux/kthread.h>

#define BUFFER_SIZE 10

struct buffer {
    int data[BUFFER_SIZE];
    int head, tail;
    struct semaphore empty;   /* Counts empty slots */
    struct semaphore full;    /* Counts full slots */
    struct mutex lock;        /* Protects buffer access */
};

static struct buffer buf;

int producer(void *arg)
{
    int item = 0;

    while (!kthread_should_stop()) {
        /* Wait for empty slot */
        if (down_interruptible(&buf.empty))
            break;

        mutex_lock(&buf.lock);
        buf.data[buf.head] = item++;
        buf.head = (buf.head + 1) % BUFFER_SIZE;
        mutex_unlock(&buf.lock);

        /* Signal full slot */
        up(&buf.full);

        msleep(100);
    }
    return 0;
}

int consumer(void *arg)
{
    int item;

    while (!kthread_should_stop()) {
        /* Wait for full slot */
        if (down_interruptible(&buf.full))
            break;

        mutex_lock(&buf.lock);
        item = buf.data[buf.tail];
        buf.tail = (buf.tail + 1) % BUFFER_SIZE;
        mutex_unlock(&buf.lock);

        /* Signal empty slot */
        up(&buf.empty);

        pr_info("Consumed: %d\n", item);
    }
    return 0;
}

void buffer_init(void)
{
    sema_init(&buf.empty, BUFFER_SIZE);  /* All slots empty */
    sema_init(&buf.full, 0);             /* No slots full */
    mutex_init(&buf.lock);
    buf.head = buf.tail = 0;
}
```

## Read-Write Semaphores

Allow multiple readers or one writer:

```c
#include <linux/rwsem.h>

/* Static initialization */
static DECLARE_RWSEM(my_rwsem);

/* Or dynamic */
struct rw_semaphore my_rwsem;
init_rwsem(&my_rwsem);

/* Reading (shared) */
down_read(&my_rwsem);
/* Multiple readers can be here */
up_read(&my_rwsem);

/* Writing (exclusive) */
down_write(&my_rwsem);
/* Only one writer, no readers */
up_write(&my_rwsem);
```

### RW Semaphore Operations

```c
/* Standard operations */
down_read(&rwsem);
down_write(&rwsem);
up_read(&rwsem);
up_write(&rwsem);

/* Try variants */
if (down_read_trylock(&rwsem)) {
    /* Got read lock */
    up_read(&rwsem);
}

if (down_write_trylock(&rwsem)) {
    /* Got write lock */
    up_write(&rwsem);
}

/* Downgrade write to read */
down_write(&rwsem);
/* ... do exclusive work ... */
downgrade_write(&rwsem);  /* Convert to read lock */
/* ... do shared work ... */
up_read(&rwsem);
```

### RW Semaphore Example

```c
struct config {
    struct rw_semaphore lock;
    int setting1;
    int setting2;
    char name[32];
};

static struct config global_config;

/* Called frequently - use read lock */
int get_setting1(void)
{
    int val;

    down_read(&global_config.lock);
    val = global_config.setting1;
    up_read(&global_config.lock);

    return val;
}

/* Called rarely - use write lock */
void set_setting1(int val)
{
    down_write(&global_config.lock);
    global_config.setting1 = val;
    up_write(&global_config.lock);
}
```

## Semaphore vs Mutex

| Feature | Mutex | Semaphore |
|---------|-------|-----------|
| Count | 1 | 1 to N |
| Owner tracking | Yes | No |
| Recursive | No | N/A |
| Priority inheritance | Some | No |
| Use case | Mutual exclusion | Resource counting |

## When to Use What

```mermaid
flowchart TD
    Need["Need synchronization"]
    Exclusive{"Exclusive<br/>access?"}
    Count{"Need to count<br/>resources?"}
    Sleep{"Can sleep?"}

    Need --> Exclusive
    Exclusive -->|Yes| Count
    Count -->|No| Mutex["Use Mutex"]
    Count -->|Yes| Sem["Use Counting Semaphore"]
    Exclusive -->|No (multiple readers)| RWSem["Use RW Semaphore"]
    Sleep -->|No| Spin["Use Spinlock"]

    style Mutex fill:#e3f2fd,stroke:#2196f3
    style Sem fill:#f3e5f5,stroke:#9c27b0
    style RWSem fill:#e8f5e9,stroke:#4caf50
```

## Summary

- Counting semaphores manage a pool of N resources
- Use `down()` to acquire, `up()` to release
- Multiple tasks can hold a counting semaphore
- For mutual exclusion, prefer mutex over binary semaphore
- Read-write semaphores allow multiple readers or one writer
- Always use interruptible variants when possible

## Next

Learn about [atomic operations]({% link part4/05-atomic-operations.md %}) for lock-free programming.
