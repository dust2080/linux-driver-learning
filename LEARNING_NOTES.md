# Linux Driver Learning Notes

**Author:** Jeff Chang  
**Last Updated:** December 12, 2025  
**Repository:** https://github.com/dust2080/linux-driver-learning

---

## 📚 Table of Contents

1. [Module 01: Hello World Module](#module-01-hello-world-module)
2. [Module 02: Character Device](#module-02-character-device)
3. [Module 03: ioctl Control](#module-03-ioctl-control)
4. [Core Concepts Reference](#core-concepts-reference)
5. [Common Patterns](#common-patterns)
6. [Troubleshooting Guide](#troubleshooting-guide)
7. [Next Steps](#next-steps)

---

## Module 01: Hello World Module

### 🎯 Learning Objectives
- Understand kernel module lifecycle
- Learn module initialization and cleanup
- Master kernel logging

### 📋 Core Concepts

#### 1. Module Structure
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init hello_init(void)
{
    pr_info("Hello: Module loaded\n");
    return 0;
}

static void __exit hello_exit(void)
{
    pr_info("Hello: Module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeff");
MODULE_DESCRIPTION("Basic kernel module");
```

#### 2. Key Functions
- `module_init()`: Register initialization function
- `module_exit()`: Register cleanup function
- `__init`: Memory optimization (freed after init)
- `__exit`: Memory optimization (freed if not used)

#### 3. Kernel Logging
```c
pr_info()   // Information
pr_warn()   // Warning
pr_err()    // Error
pr_debug()  // Debug (may not show)
```

**View logs:** `dmesg | tail` or `sudo dmesg -w`

### 💡 Key Takeaways
- Kernel modules extend kernel functionality without reboot
- Must return 0 on success, negative error code on failure
- GPL license required for most kernel symbols
- Kernel space != User space (different memory, different rules)

---

## Module 02: Character Device

### 🎯 Learning Objectives
- Create device files (`/dev/*`)
- Implement file operations (open, release, read, write)
- Safe user-kernel data transfer

### 📋 Core Concepts

#### 1. Device Registration
```c
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

// Allocate major number dynamically
dev_t dev_num;
alloc_chrdev_region(&dev_num, 0, 1, "mydev");
int major = MAJOR(dev_num);

// Initialize cdev
struct cdev my_cdev;
cdev_init(&my_cdev, &fops);
cdev_add(&my_cdev, dev_num, 1);

// Create device class and file
struct class *dev_class = class_create("myclass");
device_create(dev_class, NULL, dev_num, NULL, "mydev");
// Result: /dev/mydev created automatically
```

#### 2. File Operations Structure
```c
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};
```

#### 3. Safe Data Transfer

**❌ NEVER do this:**
```c
// WRONG! Will cause kernel panic
char *user_ptr = (char *)arg;
*user_ptr = 'A';  // Direct access - CRASH!
```

**✅ Always do this:**
```c
// Kernel → User
if (copy_to_user(user_buffer, kernel_buffer, count)) {
    return -EFAULT;
}

// User → Kernel
if (copy_from_user(kernel_buffer, user_buffer, count)) {
    return -EFAULT;
}
```

**Why?**
- User space memory may be swapped out
- User pointer may be invalid
- MMU protection prevents direct access
- Must use special functions that handle page faults

#### 4. Read Operation Pattern
```c
static ssize_t device_read(struct file *filp, char __user *buffer,
                           size_t length, loff_t *offset)
{
    // Check if EOF
    if (*offset >= data_size) {
        return 0;  // EOF
    }
    
    // Calculate bytes to copy
    size_t to_copy = min(length, data_size - *offset);
    
    // Copy to user space
    if (copy_to_user(buffer, kernel_data + *offset, to_copy)) {
        return -EFAULT;
    }
    
    // Update offset
    *offset += to_copy;
    
    return to_copy;  // Return bytes read
}
```

### 💡 Key Takeaways
- Major number identifies driver, minor identifies device instance
- `copy_to_user` / `copy_from_user` are mandatory for safety
- Must handle EOF properly (return 0 when no more data)
- Device files created automatically with `device_create()`

---

## Module 03: ioctl Control

### 🎯 Learning Objectives
- Define ioctl commands using macros
- Implement device parameter control
- Parameter validation and error handling
- Thread-safe state management

### 📋 Core Concepts

#### 1. ioctl Command Definition

**File: ioctl_cmd.h (shared between user and kernel)**
```c
#include <linux/ioctl.h>

#define IOCTL_MAGIC 'I'

// Command definitions
#define IOCTL_RESET       _IO(IOCTL_MAGIC, 0)                    // No data
#define IOCTL_SET_PARAMS  _IOW(IOCTL_MAGIC, 1, struct device_params)  // User→Kernel
#define IOCTL_GET_PARAMS  _IOR(IOCTL_MAGIC, 2, struct device_params)  // Kernel→User
#define IOCTL_GET_STATUS  _IOR(IOCTL_MAGIC, 3, struct device_status)

// Data structures
struct device_params {
    unsigned int gain;      // 0-100
    unsigned int exposure;  // 1-1000 ms
    unsigned int wb_temp;   // 2000-10000 K
};
```

**Macro Explanation:**
- `_IO(magic, nr)`: No data transfer
- `_IOR(magic, nr, type)`: Read from kernel (kernel → user)
- `_IOW(magic, nr, type)`: Write to kernel (user → kernel)
- `_IOWR(magic, nr, type)`: Read and write

#### 2. ioctl Handler Implementation

```c
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct device_params params;
    
    // Lock for thread safety
    mutex_lock(&dev_state->lock);
    
    switch (cmd) {
        case IOCTL_RESET:
            reset_device(dev_state);
            break;
            
        case IOCTL_SET_PARAMS:
            // Copy from user space
            if (copy_from_user(&params, (void __user *)arg, sizeof(params))) {
                ret = -EFAULT;
                break;
            }
            
            // Validate parameters
            ret = validate_params(&params);
            if (ret) break;
            
            // Apply parameters
            dev_state->params = params;
            pr_info("Parameters updated: gain=%u, exposure=%u, wb=%u\n",
                    params.gain, params.exposure, params.wb_temp);
            break;
            
        case IOCTL_GET_PARAMS:
            // Copy to user space
            if (copy_to_user((void __user *)arg, &dev_state->params, sizeof(params))) {
                ret = -EFAULT;
            }
            break;
            
        case IOCTL_START_STREAM:
            if (dev_state->is_streaming) {
                ret = -EBUSY;  // Already streaming
            } else {
                dev_state->is_streaming = 1;
                dev_state->frame_count = 0;
            }
            break;
            
        default:
            ret = -ENOTTY;  // Invalid ioctl command
            break;
    }
    
    mutex_unlock(&dev_state->lock);
    return ret;
}
```

#### 3. Parameter Validation
```c
static int validate_params(const struct device_params *params)
{
    if (params->gain > 100) {
        pr_err("Invalid gain %u (must be 0-100)\n", params->gain);
        return -EINVAL;
    }
    
    if (params->exposure < 1 || params->exposure > 1000) {
        pr_err("Invalid exposure %u (must be 1-1000)\n", params->exposure);
        return -EINVAL;
    }
    
    if (params->wb_temp < 2000 || params->wb_temp > 10000) {
        pr_err("Invalid white balance %u (must be 2000-10000)\n", params->wb_temp);
        return -EINVAL;
    }
    
    return 0;
}
```

#### 4. User Space Usage
```c
int fd = open("/dev/ioctl_dev", O_RDWR);

// Reset device
ioctl(fd, IOCTL_RESET, NULL);

// Set parameters
struct device_params params = {
    .gain = 75,
    .exposure = 200,
    .wb_temp = 6500
};
if (ioctl(fd, IOIOCTL_SET_PARAMS, &params) < 0) {
    perror("ioctl failed");
}

// Get parameters
if (ioctl(fd, IOCTL_GET_PARAMS, &params) < 0) {
    perror("ioctl failed");
}

printf("Current gain: %u\n", params.gain);

close(fd);
```

### 💡 Key Takeaways
- ioctl provides flexible device control beyond read/write
- Magic number prevents command conflicts between drivers
- Always validate parameters before applying
- Use mutex to protect shared device state
- Comprehensive testing includes both valid and invalid inputs

### 📊 Test Results
```
Module 03 Test Summary:
✓ Test 1: Reset Device
✓ Test 2: Get Current Parameters
✓ Test 3: Set New Parameters
✓ Test 4: Test Invalid Parameters (validation working)
✓ Test 5: Get Device Status
✓ Test 6: Start Streaming
✓ Test 7: Start Streaming Again (EBUSY correctly returned)
✓ Test 8: Stop Streaming

Total: 8/8 tests passed
Execution time: 3.2ms
```

---

## Core Concepts Reference

### 🔐 User Space vs Kernel Space

| Aspect | User Space | Kernel Space |
|--------|-----------|--------------|
| **Memory** | Virtual, pageable | Physical, pinned |
| **Access** | Limited, protected | Full hardware access |
| **Crash** | Process dies | Kernel panic (system crash) |
| **Functions** | printf, malloc | printk, kmalloc |
| **Speed** | Slower (context switch) | Faster (direct access) |

**Key Rule:** Never directly access user space pointers from kernel!

---

### ⚠️ Error Codes

| Code | Value | Meaning | When to Use |
|------|-------|---------|-------------|
| `EFAULT` | -14 | Bad address | copy_to_user/copy_from_user failed |
| `EINVAL` | -22 | Invalid argument | Parameter validation failed |
| `EBUSY` | -16 | Device busy | Resource already in use |
| `ENOTTY` | -25 | Inappropriate ioctl | Invalid ioctl command number |
| `ENOMEM` | -12 | Out of memory | kmalloc failed |
| `ENODEV` | -19 | No such device | Device not found |
| `EPERM` | -1 | Permission denied | Insufficient privileges |

**Usage in kernel:**
```c
return -EINVAL;  // Return negative error code
```

**Usage in user space:**
```c
if (ioctl(fd, CMD, arg) < 0) {
    perror("ioctl");  // Prints: "ioctl: Invalid argument"
    // or
    printf("Error: %s\n", strerror(errno));
}
```

---

### 🔒 Thread Safety: Mutex

**Why needed?**
```c
// Without mutex - RACE CONDITION!
Process A: reads counter = 5
Process B: reads counter = 5
Process A: writes counter = 6
Process B: writes counter = 6
Result: counter = 6 (should be 7!)
```

**Correct usage:**
```c
#include <linux/mutex.h>

struct device_state {
    struct mutex lock;
    int counter;
};

// Initialize
mutex_init(&dev_state->lock);

// Use
mutex_lock(&dev_state->lock);
dev_state->counter++;  // Protected operation
mutex_unlock(&dev_state->lock);
```

**Rules:**
- Always unlock what you lock
- Don't sleep while holding mutex
- Keep critical sections short
- Cleanup: `mutex_destroy()` (optional, automatic on kfree)

---

### 📝 Memory Management

#### Kernel Memory Allocation
```c
// Allocate
void *ptr = kmalloc(size, GFP_KERNEL);
if (!ptr) {
    return -ENOMEM;
}

// Use
memset(ptr, 0, size);

// Free
kfree(ptr);
```

#### GFP Flags
- `GFP_KERNEL`: Normal allocation, may sleep (use in most cases)
- `GFP_ATOMIC`: Cannot sleep (use in interrupt context)
- `GFP_DMA`: DMA-capable memory

#### Zero-initialized Allocation
```c
void *ptr = kzalloc(size, GFP_KERNEL);  // Allocated and zeroed
```

---

## Common Patterns

### Pattern 1: Device State Structure
```c
struct device_state {
    struct mutex lock;           // For thread safety
    struct device_params params; // Device parameters
    unsigned int is_active;      // State flags
    // ... other state variables
};

static struct device_state *dev_state;  // Global instance
```

### Pattern 2: Module Init Template
```c
static int __init mydriver_init(void)
{
    int ret;
    
    // 1. Allocate device state
    dev_state = kzalloc(sizeof(*dev_state), GFP_KERNEL);
    if (!dev_state)
        return -ENOMEM;
    
    // 2. Initialize state
    mutex_init(&dev_state->lock);
    
    // 3. Register device
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0)
        goto err_alloc;
    
    // 4. Setup cdev
    cdev_init(&my_cdev, &fops);
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0)
        goto err_cdev;
    
    // 5. Create device class and file
    dev_class = class_create(CLASS_NAME);
    if (IS_ERR(dev_class)) {
        ret = PTR_ERR(dev_class);
        goto err_class;
    }
    
    dev_device = device_create(dev_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(dev_device)) {
        ret = PTR_ERR(dev_device);
        goto err_device;
    }
    
    pr_info("%s: Driver initialized\n", DEVICE_NAME);
    return 0;
    
err_device:
    class_destroy(dev_class);
err_class:
    cdev_del(&my_cdev);
err_cdev:
    unregister_chrdev_region(dev_num, 1);
err_alloc:
    kfree(dev_state);
    return ret;
}
```

### Pattern 3: Module Exit Template
```c
static void __exit mydriver_exit(void)
{
    // Reverse order of init
    device_destroy(dev_class, dev_num);
    class_destroy(dev_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    kfree(dev_state);
    
    pr_info("%s: Driver removed\n", DEVICE_NAME);
}
```

---

## Troubleshooting Guide

### Problem: Module won't load
**Error:** `insmod: ERROR: could not insert module: Invalid parameters`

**Solutions:**
1. Check kernel log: `dmesg | tail`
2. Verify kernel version match: `uname -r` vs module build
3. Check for missing dependencies
4. Verify module signature (if kernel requires signed modules)

---

### Problem: Permission denied on device file
**Error:** `open: Permission denied`

**Solution:**
```bash
ls -l /dev/mydev
# If shows: crw------- (only root can access)

sudo chmod 666 /dev/mydev
# or run as root
sudo ./test_app
```

**Permanent fix:** Add udev rule or set permissions in driver:
```c
static int device_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

// In init:
dev_class->dev_uevent = device_uevent;
```

---

### Problem: Kernel panic / Oops
**Symptoms:** System freezes, kernel log shows "Oops" or "kernel panic"

**Common causes:**
1. Dereferencing NULL pointer
2. Direct access to user space pointer
3. Memory corruption
4. Stack overflow (large local variables)

**Prevention:**
```c
// Always check pointers
if (!ptr) {
    pr_err("NULL pointer!\n");
    return -EINVAL;
}

// Use copy_to_user/copy_from_user
// Don't use large stack variables
// Use kmalloc for large allocations
```

---

### Problem: Module won't unload
**Error:** `rmmod: ERROR: Module is in use`

**Solutions:**
```bash
# Check what's using it
lsmod | grep mymodule

# Check open file descriptors
lsof | grep /dev/mydev

# Make sure all applications closed the device
# Then retry
sudo rmmod mymodule
```

---

## Next Steps

### Module 04: Poll/Select (Coming Soon)
**Topics:**
- Asynchronous I/O
- Wait queues
- `poll()` and `select()` system calls
- Event notifications
- Non-blocking I/O patterns

**Key Concepts:**
- `poll_wait()` function
- Wait queue management
- Event mask (`POLLIN`, `POLLOUT`, etc.)
- Multiple file descriptor monitoring

---

### Module 05: ISP Simulator (Planned)
**Integration with ISP Pipeline project:**
- Real camera parameter control
- RAW image processing interface
- Connection to CUDA acceleration
- Professional driver architecture

**Skills demonstrated:**
- End-to-end system design
- Driver + application integration
- Performance optimization
- Real-world device simulation

---

## Quick Reference

### Compilation Commands
```bash
make                  # Build module
sudo insmod module.ko # Load module
sudo rmmod module     # Unload module
dmesg | tail         # View kernel log
lsmod | grep module  # Check if loaded
modinfo module.ko    # Show module info
```

### Useful Kernel Functions
```c
// Logging
pr_info(), pr_warn(), pr_err(), pr_debug()

// Memory
kmalloc(), kfree(), kzalloc()
copy_to_user(), copy_from_user()

// Synchronization
mutex_init(), mutex_lock(), mutex_unlock()

// Device registration
alloc_chrdev_region(), cdev_init(), cdev_add()
class_create(), device_create()
```

### File Locations
```bash
/dev/*                        # Device files
/sys/class/*/                 # Device class sysfs entries
/proc/modules                 # Loaded modules
/lib/modules/$(uname -r)/     # Kernel modules location
/var/log/kern.log             # Kernel log (on some systems)
```

---

## Resources

### Official Documentation
- [Linux Kernel Documentation](https://www.kernel.org/doc/html/latest/)
- [Device Driver Tutorial](https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html)
- [ioctl Number Registry](https://www.kernel.org/doc/html/latest/userspace-api/ioctl/ioctl-number.html)

### Books
- Linux Device Drivers, 3rd Edition (LDD3) - Free online
- Linux Kernel Development by Robert Love
- Understanding the Linux Kernel by Daniel P. Bovet

### Online Communities
- [Kernel Newbies](https://kernelnewbies.org/)
- [Linux Kernel Mailing List](https://lkml.org/)
- [Stack Overflow - Linux Kernel tag](https://stackoverflow.com/questions/tagged/linux-kernel)

---

## Learning Checklist

### Module 01: Hello World ✅
- [ ] Understand module lifecycle
- [ ] Use module_init/module_exit
- [ ] Read kernel logs with dmesg
- [ ] Set MODULE_LICENSE correctly

### Module 02: Character Device ✅
- [ ] Register character device
- [ ] Implement file operations
- [ ] Use copy_to_user/copy_from_user correctly
- [ ] Handle EOF properly
- [ ] Create device file automatically

### Module 03: ioctl Control ✅
- [ ] Define ioctl commands with macros
- [ ] Implement ioctl handler
- [ ] Validate parameters
- [ ] Use mutex for thread safety
- [ ] Handle all error cases
- [ ] Write comprehensive tests

### Module 04: Poll/Select ⏳
- [ ] Understand wait queues
- [ ] Implement poll function
- [ ] Handle multiple file descriptors
- [ ] Non-blocking I/O patterns

---

**End of Learning Notes**

*These notes summarize key concepts from Linux Driver Learning project.*  
*For complete code and detailed documentation, see: https://github.com/dust2080/linux-driver-learning*
