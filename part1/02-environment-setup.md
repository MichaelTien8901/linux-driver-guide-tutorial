---
layout: default
title: "1.2 Environment Setup"
parent: "Part 1: Getting Started"
nav_order: 3
---

# Environment Setup

We'll use Docker to create a reproducible development environment with all the tools needed for kernel driver development.

## Why Docker?

Using Docker provides several advantages:

- **Reproducibility**: Same environment on any machine
- **No host pollution**: All tools contained in the image
- **Cross-compilation ready**: ARM and x86 toolchains included
- **Easy cleanup**: Remove the container when done

## Prerequisites

Install Docker on your system:

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install docker.io docker-compose
sudo usermod -aG docker $USER
# Log out and back in for group change to take effect
```

### Fedora/RHEL

```bash
sudo dnf install docker docker-compose
sudo systemctl enable --now docker
sudo usermod -aG docker $USER
```

### macOS

Download and install [Docker Desktop](https://www.docker.com/products/docker-desktop).

### Windows

Install [Docker Desktop with WSL 2 backend](https://docs.docker.com/desktop/windows/install/).

## Project Structure

Clone the guide repository:

```bash
git clone https://github.com/yourusername/linux-driver-dev-guide
cd linux-driver-dev-guide
```

The structure looks like:

```
linux-driver-dev-guide/
├── docs/
│   ├── _config.yml       # Jekyll configuration
│   ├── Dockerfile        # Development container
│   ├── docker-compose.yml
│   ├── examples/         # Buildable code examples
│   │   └── part1/
│   │       └── hello-world/
│   └── part1/            # Documentation
│       └── ...
└── scripts/
    ├── build-examples.sh
    └── download-kernel.sh
```

## Starting the Development Container

### Using Docker Compose

```bash
cd docs
docker-compose up -d
```

This builds the image (first time only) and starts the container.

### Entering the Container

```bash
docker-compose exec kernel-dev bash
```

You're now inside the development environment with all tools ready.

### Container Contents

The container includes:

| Tool | Purpose |
|------|---------|
| `gcc` | Native x86_64 compiler |
| `gcc-aarch64-linux-gnu` | ARM64 cross-compiler |
| `gcc-arm-linux-gnueabihf` | ARM32 cross-compiler |
| `make`, `bc`, `bison`, `flex` | Build tools |
| `qemu-system-arm`, `qemu-system-x86` | Emulation |
| `sparse`, `coccinelle` | Static analysis |

## The Dockerfile

Here's what our development container looks like:

```dockerfile
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-arm-linux-gnueabihf \
    gcc-aarch64-linux-gnu \
    bc bison flex \
    libssl-dev libelf-dev libncurses-dev \
    git curl wget vim \
    qemu-system-arm qemu-system-x86 \
    device-tree-compiler \
    sparse coccinelle \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

ENV ARCH=arm64
ENV CROSS_COMPILE=aarch64-linux-gnu-

CMD ["tail", "-f", "/dev/null"]
```

## VS Code Integration

If you use VS Code, open the project and you'll be prompted to "Reopen in Container". This uses the Dev Containers extension.

Create `.devcontainer/devcontainer.json`:

```json
{
  "name": "Linux Driver Development",
  "dockerComposeFile": "../docker-compose.yml",
  "service": "kernel-dev",
  "workspaceFolder": "/workspace",
  "customizations": {
    "vscode": {
      "extensions": [
        "ms-vscode.cpptools",
        "ms-vscode.makefile-tools"
      ]
    }
  }
}
```

## Verifying the Setup

Inside the container, verify the tools are available:

```bash
# Check compilers
aarch64-linux-gnu-gcc --version
arm-linux-gnueabihf-gcc --version

# Check QEMU
qemu-system-aarch64 --version
qemu-system-x86_64 --version

# Check build tools
make --version
```

Expected output (versions may vary):

```
aarch64-linux-gnu-gcc (Ubuntu 13.2.0-23ubuntu4) 13.2.0
...
QEMU emulator version 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1)
...
GNU Make 4.3
```

## Useful Container Commands

```bash
# Stop the container
docker-compose stop

# Start the container again
docker-compose start

# Remove container (keeps image)
docker-compose down

# Rebuild image (after Dockerfile changes)
docker-compose build --no-cache

# View container logs
docker-compose logs
```

## Troubleshooting

### Permission Denied

If you get permission errors:

```bash
sudo usermod -aG docker $USER
# Then log out and back in
```

### Container Won't Start

Check Docker daemon is running:

```bash
sudo systemctl status docker
```

### Out of Disk Space

Docker images can consume significant space. Clean up:

```bash
docker system prune -a
```

{: .warning }
This removes all unused images, containers, and volumes!

## Next Steps

With your development environment ready, let's [get the kernel sources]({% link part1/03-kernel-sources.md %}).
