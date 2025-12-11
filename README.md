# Linux Driver Learning

A hands-on journey learning Linux kernel driver development from basics to advanced device control. This project demonstrates systematic progression from simple modules to complex ISP (Image Signal Processor) parameter control, connecting to my [ISP Pipeline](https://github.com/dust2080/ISP_Pipeline) project.

## 🎯 Learning Path

- [x] **[01 - Hello Module](01-hello-module/)**: Kernel module infrastructure and lifecycle
- [x] **[02 - Character Device](02-char-device/)**: Device file operations and user-kernel communication  
- [x] **[03 - ioctl Control](03-ioctl-control/)**: Advanced parameter control via ioctl commands ✨ **NEW**
- [ ] **04 - Poll/Select**: Asynchronous I/O and event notifications (Coming Soon)
- [ ] **05 - ISP Simulator**: Camera ISP pipeline driver integration

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
- Device registration with `register_chrdev` and `cdev`
- File operations structure (`file_operations`)
- Major/minor number management with `alloc_chrdev_region`
- VFS (Virtual File System) layer interaction
- Device class creation for automatic `/dev` file generation

### User-Kernel Communication
- Safe data transfer (`copy_to_user`, `copy_from_user`)
- Memory management in kernel space
- EOF (End-of-File) handling
- Device file creation and permissions

### Advanced Control via ioctl
- ioctl command definition using `_IO`, `_IOR`, `_IOW`, `_IOWR` macros
- Device parameter configuration and querying
- Parameter validation and error handling
- State management with mutex protection
- Error codes: `EFAULT`, `EINVAL`, `EBUSY`, `ENOTTY`

## 🚀 Quick Start

Each module includes detailed documentation and build instructions. Here's a typical workflow:

```bash
# Navigate to a module directory
cd 03-ioctl-control

# Build everything (kernel module + test app)
make all

# Quick test (build, load, test, show logs, unload)
make fulltest

# Or manual workflow:
make install          # Load driver
./test_app           # Run tests
sudo dmesg | tail    # Check kernel logs
make uninstall       # Unload driver

# Clean build artifacts
make clean
```

## 📦 Module Details

### 01 - Hello Module ✅
**Status:** Completed

Basic kernel module demonstrating:
- Module loading and unloading
- Kernel logging with `pr_info`
- Module metadata (`MODULE_LICENSE`, `MODULE_AUTHOR`)

[View Module 01 →](./01-hello-module/)

---

### 02 - Character Device ✅
**Status:** Completed

Character device driver with read/write operations:
- Device registration with major/minor numbers
- File operations (`open`, `release`, `read`, `write`)
- Safe user-kernel data transfer
- Buffer management and EOF handling

[View Module 02 →](./02-char-device/)

---

### 03 - ioctl Control ✅
**Status:** Completed | **Tests:** 8/8 Passed | **Execution Time:** 3.2ms

Advanced device control using ioctl commands:

**Features:**
- 6 ioctl commands: `RESET`, `GET/SET_PARAMS`, `GET_STATUS`, `START/STOP_STREAM`
- Device parameter control: gain (0-100), exposure (1-1000ms), white balance (2000-10000K)
- Comprehensive parameter validation
- Thread-safe state management with mutex
- Complete error handling (`EFAULT`, `EINVAL`, `EBUSY`)

**Test Results:**
```
✓ Test 1: Reset Device
✓ Test 2: Get Current Parameters
✓ Test 3: Set New Parameters
✓ Test 4: Test Invalid Parameters (validation working)
✓ Test 5: Get Device Status
✓ Test 6: Start Streaming
✓ Test 7: Start Streaming Again (EBUSY correctly returned)
✓ Test 8: Stop Streaming

Total: 8/8 tests passed
```

**Key Implementations:**
- `copy_from_user()` / `copy_to_user()` for safe data transfer
- Parameter validation rejecting invalid values
- Mutex protection preventing race conditions
- Professional error code usage

[View Module 03 →](./03-ioctl-control/)

---

### 04 - Poll/Select 🚧
**Status:** Coming Soon

Asynchronous I/O and event notifications:
- `poll()` and `select()` system calls
- Wait queues and event notifications
- Non-blocking I/O patterns

---

### 05 - ISP Simulator 🎯
**Status:** Planned

Integration with [ISP Pipeline](https://github.com/dust2080/ISP_Pipeline) project:
- Real camera parameter control
- RAW image processing pipeline
- CUDA acceleration integration
- Professional ISP driver architecture

## 📁 Project Structure

```
linux-driver-learning/
├── README.md                    # This file
├── 01-hello-module/
│   ├── hello.c                  # Basic kernel module
│   ├── Makefile
│   └── README.md
├── 02-char-device/
│   ├── chardev.c                # Character device driver
│   ├── Makefile
│   └── README.md
└── 03-ioctl-control/            # ✨ NEW
    ├── ioctl_cmd.h              # Shared command definitions
    ├── ioctl_driver.c           # Kernel driver implementation
    ├── test_app.c               # User space test application
    ├── Makefile                 # Build configuration
    └── README.md                # Detailed documentation
```

## 🎓 Learning Resources

### Books
- Linux Device Drivers, 3rd Edition (LDD3)
- Linux Kernel Development by Robert Love
- Understanding the Linux Kernel by Daniel P. Bovet

### Documentation
- [Linux Kernel Documentation](https://www.kernel.org/doc/html/latest/)
- [The Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/)
- [ioctl Number Registry](https://www.kernel.org/doc/html/latest/userspace-api/ioctl/ioctl-number.html)

### Online Resources
- [Kernel Newbies](https://kernelnewbies.org/)
- [Linux Driver Tutorial](https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html)
- [Linux Cross Reference (LXR)](https://elixir.bootlin.com/linux/latest/source)

## 🔍 Key Questions Explored

### Architecture
- How does VFS route system calls to driver functions?
- What's the difference between kernel space and user space?
- Why do we need major/minor numbers?
- How does ioctl differ from read/write operations?

### Implementation
- Why use `copy_to_user` instead of `memcpy`?
- How does EOF handling work in character devices?
- When should we use ioctl vs read/write?
- Why is mutex necessary for device state management?

### Error Handling
- When to return `EFAULT` vs `EINVAL`?
- How to handle concurrent device access?
- What are the standard Linux error codes?
- How to debug kernel drivers effectively?

### Best Practices  
- Proper error handling in kernel code
- Memory management and leak prevention
- Thread-safe device access with mutexes
- Kernel coding style compliance
- Comprehensive testing methodology

## 📊 Technical Achievements

### Module 01: Hello Module
- ✅ Basic module loading/unloading
- ✅ Kernel logging
- ✅ Module metadata

### Module 02: Character Device
- ✅ Device file operations
- ✅ User-kernel data transfer
- ✅ Buffer management
- ✅ EOF handling

### Module 03: ioctl Control
- ✅ 6 ioctl commands implemented
- ✅ Parameter validation (gain, exposure, white balance)
- ✅ State management with mutex
- ✅ Error handling (EFAULT, EINVAL, EBUSY, ENOTTY)
- ✅ Comprehensive testing (8/8 tests passed)
- ✅ Clean kernel logs with no errors
- ✅ Execution time: 3.2ms for all tests

## 🎯 Future Direction

This learning project will culminate in an **ISP Driver Simulator** that integrates with my [ISP Pipeline](https://github.com/dust2080/ISP_Pipeline) project, demonstrating:

- Real-world camera parameter control (exposure, AWB, gain)
- Kernel-level integration with CUDA-accelerated pipeline (1930x speedup)
- Professional device driver architecture
- Performance optimization techniques
- Complete ISP parameter control via ioctl

**Connection to ISP Pipeline:**
- ISP Pipeline: 7 modules (RAW, BLC, Demosaic, AWB, Gamma, Denoise, Sharpen)
- Driver Learning: Device control, parameter management, state handling
- Integration: Kernel driver → ISP pipeline configuration → CUDA acceleration

The goal is to show end-to-end understanding: from low-level driver development to high-performance image processing.

## 🤝 Background

This project is part of my preparation for embedded systems and camera software engineering roles. It demonstrates:

- Systematic learning approach (from basics to advanced)
- Strong fundamentals in Linux kernel development  
- Connection between driver-level control and application-level processing
- Hands-on implementation skills with real testing
- Professional code quality with comprehensive documentation

## 💡 Key Learnings

### From Module 01
- Kernel module lifecycle and infrastructure
- Difference between kernel and user space
- Kernel logging mechanisms

### From Module 02
- VFS layer and file operations
- Safe memory operations in kernel
- Device file creation and management

### From Module 03
- ioctl as a powerful control mechanism
- Parameter validation importance
- Thread safety with mutexes
- Professional error handling patterns
- Comprehensive testing methodology

## 📝 Notes

- All code follows Linux kernel coding style guidelines
- Each module is self-contained and can be built independently
- Detailed comments explain both "what" and "why"
- Testing instructions included for verification
- All tests passed with clean kernel logs
- No memory leaks or kernel panics observed

## 📄 License

GPL v2 (required for kernel modules)

## 👤 Author

**Jeff Chang** - Software Engineer  
- GitHub: [@dust2080](https://github.com/dust2080)
- Focus: Camera Systems, Embedded Linux, High-Performance Computing
- Background: 3+ years experience (MediaTek, Silicon Motion, Qbic Technology)
- Projects: [ISP Pipeline](https://github.com/dust2080/ISP_Pipeline) (1930x CUDA speedup)

---

*Last Updated: December 12, 2025*
*Module 03 completed with all tests passing*
