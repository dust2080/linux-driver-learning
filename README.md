# Linux Driver Learning

A hands-on journey learning Linux kernel driver development from basics to advanced device control. This project demonstrates systematic progression from simple modules to complex ISP (Image Signal Processor) parameter control, connecting to my [ISP Pipeline](https://github.com/dust2080/ISP_Pipeline) project.

## 🎯 Learning Path
 01 - Hello Module: Kernel module infrastructure and lifecycle
 02 - Character Device: Device file operations and user-kernel communication
 03 - ioctl Control: Advanced parameter control via ioctl commands
 04 - Poll/Select: Asynchronous I/O and event notifications ✅ COMPLETED
 05 - Interrupt Handling: Hardware interrupt integration with wait queues ✅ COMPLETED
 06 - DMA Concept: Direct Memory Access understanding ✅ COMPLETED
 07 - Network Streaming: TCP/IP integration with ISP Pipeline ✅ COMPLETED

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

---

### 04 - Poll/Select ✅
**Status:** Completed | **Tests:** All Passed

Asynchronous I/O with wait queues:

**Features:**
- `poll()` system call implementation
- Wait queue mechanism for process sleeping/waking
- Non-blocking I/O patterns
- Event notification system

**Key Implementations:**
- `poll_wait()` for registering to wait queue
- `wake_up_interruptible()` for waking processes
- Proper event mask handling (`POLLIN`, `POLLOUT`)
- Integration with file operations

**What I learned:**
- Wait queues are the foundation of async I/O
- `poll_wait()` doesn't actually sleep - it just registers
- The actual sleep happens when `poll()` returns 0
- But who calls `wake_up()`? → This led me to Module 05

[View Module 04 →](./04-poll-select/)

---

### 05 - Interrupt Handling ✅
**Status:** Completed | **Tests:** All Passed | **Wake-up Latency:** <0.2ms

Hardware interrupt integration completing the async I/O puzzle:

**Features:**
- Timer-based interrupt simulation (every 2 seconds)
- Complete interrupt-driven I/O flow
- Integration with Module 04's wait queue
- Sub-millisecond wake-up latency

**Key Implementations:**
- Kernel timer setup with `timer_setup()` and `mod_timer()`
- Interrupt handler simulation
- `wake_up_interruptible()` in interrupt context
- Complete poll → sleep → interrupt → wake → read flow

**Test Results:**
- Successfully captured 5+ frames
- Wake-up latency: ~0.12ms (from interrupt to poll return)
- Timer accuracy: 2.000s ±0.001s
- Perfect integration with wait queue mechanism

**Real-world Application:**
This is exactly how camera drivers work:
```
GPIO Interrupt (camera ready) → interrupt_handler() 
                               → wake_up() 
                               → poll() returns 
                               → read frame data
```

**Key Learning:**
- Module 04 provided the **wait mechanism** (poll_wait)
- Module 05 provided the **wake mechanism** (interrupt)
- Together they form **complete async I/O**

[View Module 05 →](./05-interrupt-handling/)

---

### 06 - DMA Concept ✅
**Status:** Completed | **Concept Understanding**

Direct Memory Access (DMA) concept and application in camera drivers:

**Key Concepts:**
- DMA as independent hardware (like camera sensor, network card)
- DMA vs CPU copy: 100% CPU → 5% CPU usage for large data transfers
- Two interrupts: Camera Ready + DMA Complete
- Integration with ISP Pipeline architecture

**What I Learned:**
- DMA Controller is separate hardware with its own driver
- In camera systems: Sensor → DMA → Kernel Buffer → User Space (ISP)
- Linux DMA Engine Framework provides APIs (don't write DMA driver yourself)
- Real-world data volume: 1080p @ 30fps = 120 MB/second

**Real-world Application:**
```
Camera Sensor (4MB frame)
    ↓ Interrupt #1: Frame captured
CPU configures DMA
    ↓ DMA hardware transfers (CPU does other work)
    ↓ Interrupt #2: Transfer complete
wake_up() → User Space ISP Pipeline
```

**Interview-Ready Explanation:**
- Can explain why DMA is needed for high-bandwidth devices
- Understand the complete data flow from hardware to application
- Know when to use DMA vs CPU copy
- Connect to ISP Pipeline project (driver → user space processing)

[View Module 06 →](./06-dma-concept/)

---

### 07 - Network Streaming & ISP Integration ✅
Status: Completed | Frames Processed: 5 | Total Latency: <100ms

Complete end-to-end camera system:

Features:
- TCP socket server/client architecture
- Real 640×480 RAW12 frame streaming (614,400 bytes/frame)
- poll()-based blocking I/O (CPU-efficient)
- Integration with ISP Pipeline (7-stage processing)
- Automatic 5-frame capture and processing

Key Implementations:
- Client-server TCP communication
- Large binary data transfer
- Cross-machine integration (VM → macOS)
- ISP Pipeline invocation from C++
- Complete driver-to-image workflow

Architecture:
```
Driver (VM) → frame_streamer → Network → frame_receiver (macOS) → ISP Pipeline → PNG
```

Test Results:
- ✓ 5 frames successfully transmitted
- ✓ Each frame unique (verified via MD5)
- ✓ Complete ISP processing (RAW → PNG)
- ✓ Network transfer: ~50ms/frame
- ✓ ISP processing: ~2ms/frame

Real-world Application: This demonstrates Edge-to-Cloud architecture where:
- Edge device (driver) captures sensor data
- Network transmits to processing server
- Cloud (ISP) performs compute-intensive operations
- Complete decoupling of capture and processing

What I learned:
- TCP socket programming in C/C++
- Large data transfer strategies
- System integration across machines
- Blocking I/O with poll() (Module 04+05 combined)
- Real-world camera system architecture

[View Module 07 →](07-network-streaming/)

## 📁 Project Structure
```
linux-driver-learning/
├── README.md                    # This file
├── LEARNING_NOTES.md            # Comprehensive learning notes
├── 01-hello-module/
│   ├── hello.c                  # Basic kernel module
│   ├── Makefile
│   └── README.md
├── 02-char-device/
│   ├── chardev.c                # Character device driver
│   ├── Makefile
│   └── README.md
├── 03-ioctl-control/
│   ├── ioctl_cmd.h              # Shared command definitions
│   ├── ioctl_driver.c           # Kernel driver implementation
│   ├── test_app.c               # User space test application
│   ├── Makefile
│   └── README.md
├── 04-poll-select/              ✅ COMPLETED
│   ├── poll_driver.c            # Wait queue implementation
│   ├── poll_test.c              # Poll test program
│   ├── Makefile
│   └── README.md
├── 05-interrupt-handling/       ✅ COMPLETED
│   ├── v1_timer_interrupt.c     # Basic interrupt handler
│   ├── v2_with_waitqueue.c      # Complete integration
│   ├── interrupt_test.c         # User space test
│   ├── Makefile
│   ├── README.md
│   └── learning_notes.md        # Detailed module notes
├── 06-dma-concept/              ✅ COMPLETED
│   └── README.md                # DMA concept explanation
└── 07-network-streaming/        ✅ COMPLETED
    ├── frame_streamer.c         # TCP server (VM side)
    ├── Makefile
    ├── README.md
    └── test/
        └── tcp_server.py        # Network test script
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

### Module 04: Poll/Select
- ✅ poll() system call implementation
- ✅ Wait queue mechanism
- ✅ Non-blocking I/O
- ✅ Event notification system
- ✅ Understanding of async I/O foundations

### Module 05: Interrupt Handling
- ✅ Timer-based interrupt simulation
- ✅ Interrupt handler implementation
- ✅ Complete integration with wait queue
- ✅ Sub-millisecond wake-up latency (<0.2ms)
- ✅ Understanding of real camera driver workflow
- ✅ Atomic context constraints mastered

### Module 06: DMA Concept
- ✅ Understanding DMA as independent hardware
- ✅ DMA vs CPU copy trade-offs (100% → 5% CPU)
- ✅ Two-interrupt model (Camera Ready + DMA Complete)
- ✅ Integration with camera driver architecture
- ✅ Connection to ISP Pipeline data flow
- ✅ Interview-ready explanations
- ✅ Real-world data volume calculations (120 MB/s)

### Module 07: Network Streaming
- ✅ TCP socket server/client implementation
- ✅ poll()-based blocking I/O (CPU-efficient waiting)
- ✅ 640×480 RAW12 frame transmission (614,400 bytes/frame)
- ✅ Cross-machine integration (VM → macOS)
- ✅ ISP Pipeline integration via network
- ✅ Automatic 5-frame capture and processing
- ✅ Complete driver-to-PNG workflow
- ✅ Network transfer: ~50ms/frame
- ✅ Total latency: <100ms (capture to processed image)
- ✅ Edge-to-Cloud architecture demonstration
- ✅ Large binary data transfer strategies
- ✅ System integration across different machines

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

*Last Updated: December 16, 2025*
*Module 07 completed - Network Streaming & ISP Integration
