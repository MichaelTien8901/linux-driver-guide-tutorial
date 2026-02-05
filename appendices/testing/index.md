---
layout: default
title: "Appendix E: Testing"
nav_order: 21
has_children: true
---

# Appendix E: Testing

Testing strategies for kernel drivers.

## Testing Layers

```mermaid
flowchart TB
    subgraph Unit["Unit Testing"]
        KUNIT["KUnit"]
    end

    subgraph Runtime["Runtime Checks"]
        KASAN["KASAN (memory)"]
        LOCKDEP["lockdep (locking)"]
        UBSAN["UBSAN (undefined)"]
    end

    subgraph Integration["Integration"]
        SELFTEST["kselftest"]
        SYZKALLER["syzkaller (fuzzing)"]
    end

    Unit --> Runtime
    Runtime --> Integration

    style Unit fill:#7a8f73,stroke:#2e7d32
    style Runtime fill:#738f99,stroke:#0277bd
    style Integration fill:#8f8a73,stroke:#f9a825
```

## Quick Start

| Tool | Purpose | Enable |
|------|---------|--------|
| KUnit | Unit tests | `CONFIG_KUNIT=y` |
| KASAN | Memory bugs | `CONFIG_KASAN=y` |
| lockdep | Lock issues | `CONFIG_PROVE_LOCKING=y` |
| UBSAN | Undefined behavior | `CONFIG_UBSAN=y` |

## Chapters

| Chapter | What You'll Learn |
|---------|-------------------|
| [KUnit]({% link appendices/testing/01-kunit.md %}) | Writing unit tests |
| [Runtime Checks]({% link appendices/testing/02-runtime-checks.md %}) | KASAN, lockdep, UBSAN |

## Example

- **[KUnit Test](../examples/appendices/kunit-test/)** - Example test module

## Recommended Config

```kconfig
# Debug options
CONFIG_DEBUG_INFO=y
CONFIG_DEBUG_KERNEL=y

# Memory debugging
CONFIG_KASAN=y
CONFIG_KASAN_INLINE=y

# Lock debugging
CONFIG_PROVE_LOCKING=y
CONFIG_DEBUG_LOCK_ALLOC=y

# Undefined behavior
CONFIG_UBSAN=y

# Unit testing
CONFIG_KUNIT=y
```

## Further Reading

- [Kernel Testing Guide](https://docs.kernel.org/dev-tools/testing-overview.html)
- [KUnit](https://docs.kernel.org/dev-tools/kunit/index.html)
- [KASAN](https://docs.kernel.org/dev-tools/kasan.html)
