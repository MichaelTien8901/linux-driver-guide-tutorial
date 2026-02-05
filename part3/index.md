---
layout: default
title: "Part 3: Character Device Drivers"
nav_order: 4
has_children: true
---

# Part 3: Character Device Drivers

Character devices are the most common type of driver and provide the foundation for interacting with hardware through the filesystem interface. In this part, you'll learn how to create complete character device drivers that applications can open, read, write, and control.

## What You'll Learn

```mermaid
flowchart LR
    subgraph Application
        App["User Program"]
    end

    subgraph VFS["Virtual File System"]
        FD["File Descriptor"]
    end

    subgraph Driver["Your Driver"]
        FO["file_operations"]
        CD["cdev"]
    end

    subgraph Hardware
        HW["Device"]
    end

    App -->|"open(/dev/mydev)"| FD
    FD --> FO
    FO --> CD
    CD --> HW

    style Application fill:#7a8f73,stroke:#2e7d32
    style Driver fill:#8f8a73,stroke:#f9a825
```

## Chapter Contents

| Chapter | Topic | Key Concepts |
|---------|-------|--------------|
| [3.1]({% link part3/01-char-device-concepts.md %}) | Character Device Concepts | Major/minor numbers, device nodes, /dev |
| [3.2]({% link part3/02-file-operations.md %}) | File Operations | file_operations structure, implementing handlers |
| [3.3]({% link part3/03-cdev-registration.md %}) | Device Registration | cdev_init, cdev_add, device_create |
| [3.4]({% link part3/04-read-write.md %}) | Read and Write | copy_to_user, copy_from_user, data transfer |
| [3.5]({% link part3/05-ioctl.md %}) | IOCTL | Custom commands, _IO macros |
| [3.6]({% link part3/06-poll-select.md %}) | Poll and Seek | poll(), llseek(), blocking I/O |
| [3.7]({% link part3/07-misc-devices.md %}) | Misc Devices | Simplified character device registration |

## Prerequisites

Before starting this part, ensure you understand:

- Module lifecycle (Part 2)
- Kernel memory access rules
- Basic kernel coding style

## Examples

This part includes complete working examples:

- **simple-char**: Basic character device with read/write
- **ioctl-device**: Device with custom ioctl commands

## Character Device Overview

Character devices:

- Transfer data as a stream of bytes (no block structure)
- Appear as special files in `/dev/`
- Support file operations: open, read, write, close, ioctl
- Are identified by major and minor numbers

Common character devices include:

- Serial ports (`/dev/ttyS*`)
- Terminals (`/dev/tty*`)
- Input devices (`/dev/input/*`)
- Memory devices (`/dev/mem`, `/dev/null`, `/dev/zero`)
- Custom hardware interfaces

## The Big Picture

```mermaid
flowchart TB
    subgraph UserSpace["User Space"]
        App["Application"]
        LibC["glibc: open(), read(), write()"]
        DevFile["/dev/mydevice"]
    end

    subgraph KernelSpace["Kernel Space"]
        Syscall["System Call Handler"]
        VFS["Virtual File System"]
        CharDev["Character Device Layer"]
        subgraph YourDriver["Your Driver"]
            FO["file_operations"]
            Open["my_open()"]
            Read["my_read()"]
            Write["my_write()"]
            Ioctl["my_ioctl()"]
        end
    end

    App --> LibC
    LibC --> DevFile
    DevFile --> Syscall
    Syscall --> VFS
    VFS --> CharDev
    CharDev --> FO
    FO --> Open
    FO --> Read
    FO --> Write
    FO --> Ioctl

    style UserSpace fill:#7a8f73,stroke:#2e7d32
    style KernelSpace fill:#8f6d72,stroke:#c62828
    style YourDriver fill:#8f8a73,stroke:#f9a825
```

## Further Reading

- [Character Device Drivers](https://docs.kernel.org/driver-api/infrastructure.html) - Kernel driver infrastructure
- [VFS Documentation](https://docs.kernel.org/filesystems/vfs.html) - Virtual filesystem layer
- [Linux Device Drivers, 3rd Ed - Chapter 3](https://lwn.net/Kernel/LDD3/) - Character drivers

## Next

Start with [Character Device Concepts]({% link part3/01-char-device-concepts.md %}) to understand how character devices are identified and accessed.
