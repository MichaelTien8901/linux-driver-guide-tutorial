FROM ubuntu:24.04

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install kernel development tools and cross-compilers
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-arm-linux-gnueabihf \
    gcc-aarch64-linux-gnu \
    bc \
    bison \
    flex \
    libssl-dev \
    libelf-dev \
    libncurses-dev \
    git \
    curl \
    wget \
    vim \
    cpio \
    kmod \
    qemu-system-arm \
    qemu-system-x86 \
    device-tree-compiler \
    u-boot-tools \
    python3 \
    python3-pip \
    sparse \
    coccinelle \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Create directory for kernel sources
RUN mkdir -p /kernel

# Environment variables for cross-compilation
ENV ARCH=arm64
ENV CROSS_COMPILE=aarch64-linux-gnu-

# Keep container running
CMD ["tail", "-f", "/dev/null"]
