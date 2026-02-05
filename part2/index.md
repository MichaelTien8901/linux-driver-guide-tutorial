---
layout: default
title: "Part 2: Kernel Fundamentals"
nav_order: 3
has_children: true
---

# Part 2: Kernel Fundamentals

This part covers the essential concepts every kernel developer must understand: architecture, module lifecycle, coding conventions, and error handling patterns.

## What You'll Learn

- Kernel vs user space architecture
- Module initialization and cleanup
- Module parameters and symbol exporting
- Kernel coding style and conventions
- Kernel data types and endianness
- Error handling patterns

## Prerequisites

Complete [Part 1: Getting Started]({% link part1/index.md %}) or have a working development environment.

## Chapter Overview

| Chapter | Topic | Description |
|---------|-------|-------------|
| [2.1]({% link part2/01-kernel-architecture.md %}) | Kernel Architecture | User space vs kernel space, privilege levels |
| [2.2]({% link part2/02-module-lifecycle.md %}) | Module Lifecycle | init/exit, module_param, EXPORT_SYMBOL |
| [2.3]({% link part2/03-coding-style.md %}) | Coding Style | Kernel conventions, checkpatch.pl |
| [2.4]({% link part2/04-data-types.md %}) | Data Types | Fixed-width types, endianness handling |
| [2.5]({% link part2/05-error-handling.md %}) | Error Handling | ERR_PTR, goto cleanup, return conventions |
| [2.6]({% link part2/06-kernel-api.md %}) | Kernel API | Common macros and helper functions |

## Key Concepts

### The Kernel Environment

The kernel environment differs significantly from user space:

| Aspect | User Space | Kernel Space |
|--------|-----------|--------------|
| Memory | Virtual, protected | Direct physical access |
| Stack | Large (MB) | Small (8-16 KB) |
| Libraries | Full C library | Kernel-only APIs |
| Errors | Exceptions possible | Must handle explicitly |
| Floating point | Available | Generally unavailable |

### Why This Matters

Understanding these fundamentals helps you:

- Write stable, crash-free drivers
- Avoid common kernel programming pitfalls
- Follow kernel conventions for upstream acceptance
- Debug issues more effectively

## Examples

This part includes two example modules:

- `examples/part2/module-params/` - Module parameters demonstration
- `examples/part2/export-symbol/` - Symbol exporting between modules

## Further Reading

- [Kernel Coding Style](https://www.kernel.org/doc/html/latest/process/coding-style.html) - Official style guide
- [Kernel API Documentation](https://docs.kernel.org/core-api/index.html) - Core kernel APIs
- [Driver API](https://docs.kernel.org/driver-api/index.html) - Driver development APIs

{: .tip }
Keep the kernel coding style guide bookmarked: [Documentation/process/coding-style.rst](https://www.kernel.org/doc/html/latest/process/coding-style.html)
