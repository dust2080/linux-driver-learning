# Module 03: ioctl Control Driver

A Linux character device driver demonstrating ioctl (I/O Control) functionality for device parameter control. This driver simulates an ISP (Image Signal Processor) device with configurable parameters.

## Overview

This module demonstrates:
- **ioctl command definitions** using `_IO`, `_IOR`, `_IOW`, `_IOWR` macros
- **User-kernel data transfer** with `copy_from_user()` and `copy_to_user()`
- **Parameter validation** and error handling
- **Device state management** with mutex protection
- **Comprehensive testing** of all ioctl operations

## Features

### Device Parameters
- **Gain** (0-100): Simulates ISO sensitivity
- **Exposure** (1-1000 ms): Simulates shutter speed
- **White Balance** (2000-10000 K): Color temperature control

### ioctl Commands
1. `IOCTL_RESET` - Reset device to default parameters
2. `IOCTL_SET_PARAMS` - Set device parameters
3. `IOCTL_GET_PARAMS` - Get current parameters
4. `IOCTL_GET_STATUS` - Get device status (streaming state, frame count)
5. `IOCTL_START_STREAM` - Start streaming/recording
6. `IOCTL_STOP_STREAM` - Stop streaming/recording

## File Structure

```
03-ioctl-control/
├── ioctl_cmd.h         # Shared ioctl command definitions
├── ioctl_driver.c      # Kernel driver implementation
├── test_app.c          # User space test application
├── Makefile            # Build configuration
└── README.md           # This file
```

## Build Instructions

### Prerequisites
```bash
# Install kernel headers
sudo apt-get install linux-headers-$(uname -r)

# Install build tools (if not already installed)
sudo apt-get install build-essential
```

### Build Everything
```bash
make all
```

This will:
- Build the kernel module (`ioctl_driver.ko`)
- Build the test application (`test_app`)

### Build Individual Components
```bash
make module    # Build kernel module only
make app       # Build test application only
```

## Usage

### Quick Test (Recommended)
```bash
# Run complete test cycle
make fulltest
```

This will automatically:
1. Clean previous builds
2. Compile everything
3. Load the driver
4. Run all tests
5. Show kernel logs
6. Unload the driver

### Manual Testing

#### Step 1: Load the Driver
```bash
make install
# or manually:
sudo insmod ioctl_driver.ko
```

Verify the device file was created:
```bash
ls -l /dev/ioctl_dev
```

#### Step 2: Run Tests
```bash
make test
# or manually:
./test_app
```

#### Step 3: Check Kernel Logs
```bash
make log
# or manually:
sudo dmesg | tail -30
```

#### Step 4: Unload the Driver
```bash
make uninstall
# or manually:
sudo rmmod ioctl_driver
```

## Test Application

The test application (`test_app`) performs 8 comprehensive tests:

1. **Reset Test**: Resets device to default parameters
2. **Get Parameters Test**: Reads current parameters
3. **Set Parameters Test**: Sets and verifies new parameters
4. **Invalid Parameters Test**: Tests parameter validation
5. **Get Status Test**: Reads device status
6. **Start Streaming Test**: Starts device streaming
7. **Duplicate Start Test**: Verifies EBUSY error handling
8. **Stop Streaming Test**: Stops device streaming

### Expected Output
```
========================================
ioctl Driver Test Application
========================================

Opening device /dev/ioctl_dev...
✓ Device opened successfully (fd=3)

========================================
Test 1: Reset Device
========================================
✓ Device reset successfully

[... more tests ...]

========================================
Test Summary
========================================
Total tests:  8
Passed:       8
Failed:       0

✓ All tests passed!
```

## Key Concepts Demonstrated

### 1. ioctl Command Definition
```c
/* No data transfer */
#define IOCTL_RESET  _IO(IOCTL_MAGIC, 0)

/* User -> Kernel */
#define IOCTL_SET_PARAMS  _IOW(IOCTL_MAGIC, 1, struct device_params)

/* Kernel -> User */
#define IOCTL_GET_PARAMS  _IOR(IOCTL_MAGIC, 2, struct device_params)
```

### 2. Safe Data Transfer
```c
/* Copy from user space to kernel space */
if (copy_from_user(&params, (void __user *)arg, sizeof(params))) {
    return -EFAULT;  /* Bad address */
}

/* Copy from kernel space to user space */
if (copy_to_user((void __user *)arg, &params, sizeof(params))) {
    return -EFAULT;
}
```

### 3. Thread Safety
```c
/* Protect device state with mutex */
mutex_lock(&dev_state->lock);
/* ... modify device state ... */
mutex_unlock(&dev_state->lock);
```

### 4. Error Codes
- `EFAULT (-14)`: Bad memory address
- `EINVAL (-22)`: Invalid argument
- `EBUSY (-16)`: Device or resource busy
- `ENOTTY (-25)`: Invalid ioctl command

## Kernel Log Messages

When running the driver, you can monitor kernel messages:

```bash
sudo dmesg -w  # Watch in real-time
```

Example output:
```
[  123.456] ioctl_dev: Initializing driver
[  123.457] ioctl_dev: Registered with major number 240
[  123.458] ioctl_dev: Driver initialized successfully
[  123.459] ioctl_dev: Device created at /dev/ioctl_dev
[  123.460] ioctl_dev: Device opened
[  123.461] ioctl_dev: IOCTL_RESET called
[  123.462] ioctl_dev: Device reset to defaults (gain=50, exposure=33, wb_temp=5500)
```

## Troubleshooting

### Device file not created
```bash
# Check if module is loaded
lsmod | grep ioctl_driver

# Check kernel logs for errors
sudo dmesg | grep ioctl_dev
```

### Permission denied when opening device
```bash
# Check device permissions
ls -l /dev/ioctl_dev

# If needed, change permissions
sudo chmod 666 /dev/ioctl_dev
```

### Module loading fails
```bash
# Check for missing kernel headers
sudo apt-get install linux-headers-$(uname -r)

# Check for kernel version mismatch
uname -r
modinfo ioctl_driver.ko | grep vermagic
```

## Clean Up

Remove all build artifacts:
```bash
make clean
```

## Learning Resources

- [Linux Device Drivers, 3rd Edition](https://lwn.net/Kernel/LDD3/)
- [The Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/)
- [Linux ioctl Documentation](https://www.kernel.org/doc/html/latest/userspace-api/ioctl/ioctl-number.html)

## Related Modules

- **Module 01**: Hello World - Basic kernel module
- **Module 02**: Character Device - Basic read/write operations
- **Module 04**: Poll/Select - Asynchronous I/O notifications (upcoming)

## Author

Jeff Chang - Learning Linux kernel driver development

## License

GPL v2 - Same as the Linux kernel
