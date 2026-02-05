---
layout: default
title: "Part 1: Getting Started"
nav_order: 2
has_children: true
---

# Part 1: Getting Started

Welcome to Linux kernel driver development! This section will help you set up your development environment and write your first kernel module.

## What You'll Learn

- Understanding the Linux driver development landscape
- Setting up a Docker-based development environment
- Obtaining and configuring kernel sources
- Writing, building, and loading your first kernel module
- Testing modules with QEMU (no hardware needed!)

## Prerequisites

Before starting, you should have:

- **C programming knowledge**: Comfortable with pointers, structs, and memory management
- **Basic Linux skills**: Command line navigation, file editing, shell basics
- **Docker installed**: We'll use containers for reproducible builds

No specialized hardware is required - we'll test everything in QEMU.

## Chapter Overview

| Chapter | Topic | Description |
|---------|-------|-------------|
| [1.1]({% link part1/01-introduction.md %}) | Introduction | Driver development landscape and this guide's approach |
| [1.2]({% link part1/02-environment-setup.md %}) | Environment Setup | Docker-based development environment |
| [1.3]({% link part1/03-kernel-sources.md %}) | Kernel Sources | Obtaining and configuring the kernel |
| [1.4]({% link part1/04-first-module.md %}) | First Module | Hello World kernel module |
| [1.5]({% link part1/05-qemu-testing.md %}) | QEMU Testing | Testing without physical hardware |

## Quick Start

If you're eager to get started:

```bash
# Clone the repository
git clone https://github.com/yourusername/linux-driver-dev-guide
cd linux-driver-dev-guide/docs

# Start the development container
docker-compose up -d
docker-compose exec kernel-dev bash

# Build your first module
cd /workspace/examples/part1/hello-world
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

{: .tip }
Every concept in this guide includes working code examples you can build and test immediately.

## Further Reading

- [Kernel Documentation](https://docs.kernel.org/) - Official kernel docs
- [Linux Device Drivers, 3rd Edition](https://lwn.net/Kernel/LDD3/) - Classic reference (free online)
- [Kernel Newbies](https://kernelnewbies.org/) - Beginner-friendly resources
