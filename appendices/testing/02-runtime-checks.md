---
layout: default
title: "E.2 Runtime Checks"
parent: "Appendix E: Testing"
nav_order: 2
---

# Runtime Checks

Kernel sanitizers that detect bugs at runtime.

## KASAN (Memory Bugs)

Detects use-after-free, buffer overflows, etc.

```kconfig
CONFIG_KASAN=y
CONFIG_KASAN_INLINE=y
```

When KASAN detects a bug:
```
BUG: KASAN: use-after-free in my_function+0x42
Write of size 4 at addr ffff88...
Call Trace:
  my_function+0x42
  ...
```

## lockdep (Locking Bugs)

Detects deadlocks and lock order violations.

```kconfig
CONFIG_PROVE_LOCKING=y
CONFIG_DEBUG_LOCK_ALLOC=y
```

Detects:
- AB-BA deadlocks (lock ordering)
- Recursive locking errors
- Lock in wrong context (sleep with spinlock)

Example output:
```
WARNING: possible circular locking dependency detected
  task/123 is trying to acquire lock B
  but task already holds lock A
  while lock B was acquired with A held
```

## UBSAN (Undefined Behavior)

Catches undefined behavior.

```kconfig
CONFIG_UBSAN=y
CONFIG_UBSAN_BOUNDS=y
```

Detects:
- Integer overflow
- Array out-of-bounds
- Misaligned access
- Null pointer dereference

## KMSAN (Uninitialized Memory)

Detects use of uninitialized memory.

```kconfig
CONFIG_KMSAN=y
```

## Using During Development

```bash
# Build debug kernel
make menuconfig
# Enable: Kernel hacking → Memory Debugging → KASAN
# Enable: Kernel hacking → Lock Debugging → Prove locking

make -j$(nproc)

# Run your driver
# Check dmesg for warnings
dmesg | grep -E '(KASAN|lockdep|UBSAN)'
```

## Quick Reference

| Sanitizer | Finds | Overhead |
|-----------|-------|----------|
| KASAN | Memory bugs | ~2x slower |
| lockdep | Lock bugs | ~5% slower |
| UBSAN | Undefined behavior | ~10% slower |
| KMSAN | Uninit memory | ~3x slower |

{: .tip }
> Enable all sanitizers during development. Disable for production.

## Further Reading

- [KASAN](https://docs.kernel.org/dev-tools/kasan.html)
- [lockdep](https://docs.kernel.org/locking/lockdep-design.html)
- [UBSAN](https://docs.kernel.org/dev-tools/ubsan.html)
