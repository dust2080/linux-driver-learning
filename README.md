# Linux Driver Learning

A hands-on journey learning Linux kernel driver development from basics to advanced device control. This project demonstrates systematic progression from simple modules to complex ISP (Image Signal Processor) parameter control, connecting to my [ISP Pipeline](https://github.com/dust2080/ISP_Pipeline) project.

## 🎯 Learning Path

- [x] **[01 - Hello Module](01-hello-module/)**: Kernel module infrastructure and lifecycle
- [x] **[02 - Character Device](02-char-device/)**: Device file operations and user-kernel communication  
- [ ] **03 - ioctl Control**: Advanced parameter control via ioctl (In Progress)
- [ ] **04 - ISP Simulator**: Camera ISP pipeline driver integration

## 🛠️ Environment

- **OS**: Ubuntu 24.04 ARM64
- **Kernel**: 6.14.0-36-generic
- **Compiler**: GCC 13.3.0
- **Architecture**: ARM64 (Apple Silicon VM)

## 📚 Core Concepts Covered

### Module Development
- Module initialization and cleanup (`module_init`, `module_exit`)
- Kernel logging with `printk` and log levels
- Module metadata and licensing

### Character Device Drivers  
- Device registration with `register_chrdev`
- File operations structure (`file_operations`)
- Major/minor number management
- VFS (Virtual File System) layer interaction

### User-Kernel Communication
- Safe data transfer (`copy_to_user`, `copy_from_user`)
- Memory management in kernel space
- EOF (End-of-File) handling
- Device file creation (`/dev/*`)

### Advanced Control (Planned)
- ioctl command interface
- Parameter configuration
- Status querying
- ISP pipeline integration

## 🚀 Quick Start

Each module includes detailed documentation and build instructions. Here's a typical workflow:

```bash
# Navigate to a module directory
cd 02-char-device

# Build the module
make

# Load module and create device file
make load

# Test the device
echo "Hello kernel" > /dev/hello
cat /dev/hello

# Check kernel logs
dmesg | tail

# Unload module
make unload

# Clean build artifacts
make clean
```

## 📖 Project Structure

```
linux-driver-learning/
├── README.md                    # This file
├── 01-hello-module/
│   ├── hello.c                  # Basic kernel module
│   ├── Makefile
│   └── README.md                # Module-specific documentation
├── 02-char-device/
│   ├── chardev.c                # Character device driver
│   ├── Makefile
│   └── README.md
└── 03-ioctl-control/           # Coming soon
    ├── ioctl_driver.c
    ├── ioctl_cmd.h
    ├── test_app.c
    ├── Makefile
    └── README.md
```

## 🎓 Learning Resources

### Books
- Linux Device Drivers, 3rd Edition (LDD3)
- Linux Kernel Development by Robert Love

### Documentation
- [Linux Kernel Documentation](https://www.kernel.org/doc/html/latest/)
- [The Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/)

### Online Resources
- [Kernel Newbies](https://kernelnewbies.org/)
- [Linux Driver Tutorial](https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html)

## 🔍 Key Questions Explored

### Architecture
- How does VFS route system calls to driver functions?
- What's the difference between kernel space and user space?
- Why do we need major/minor numbers?

### Implementation
- Why use `copy_to_user` instead of `memcpy`?
- How does EOF handling work in character devices?
- When should we use ioctl vs read/write?

### Best Practices  
- Proper error handling in kernel code
- Memory management and leak prevention
- Thread-safe device access
- Kernel coding style compliance

## 🎯 Future Direction

This learning project will culminate in an **ISP Driver Simulator** that integrates with my [ISP Pipeline](https://github.com/dust2080/ISP_Pipeline) project, demonstrating:

- Real-world camera parameter control (exposure, AWB, gain)
- Kernel-level integration with CUDA-accelerated pipeline
- Professional device driver architecture
- Performance optimization techniques

The goal is to show end-to-end understanding: from low-level driver development to high-performance image processing.

## 🤝 Background

This project is part of my preparation for embedded systems and camera software engineering roles, particularly for Google Camera System Software Engineer position. It demonstrates:

- Systematic learning approach
- Strong fundamentals in Linux kernel development  
- Connection between driver-level control and application-level processing
- Hands-on implementation skills

## 📝 Notes

- All code follows Linux kernel coding style guidelines
- Each module is self-contained and can be built independently
- Detailed comments explain both "what" and "why"
- Testing instructions included for verification

## 📄 License

GPL v2 (required for kernel modules)

## 👤 Author

**Jeff** - Embedded/Firmware Engineer  
- GitHub: [@dust2080](https://github.com/dust2080)
- Focus: Camera Systems, Embedded Linux, High-Performance Computing

---

*Last Updated: December 2025*
