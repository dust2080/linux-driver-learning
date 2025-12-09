# 01 - Hello World Kernel Module

A minimal kernel module demonstrating the basic infrastructure of Linux kernel modules.

## 🎯 Learning Objectives

- Understanding module initialization and cleanup flow
- Using `printk()` for kernel logging
- Module loading/unloading with `insmod`/`rmmod`
- Checking kernel logs with `dmesg`

## 🔑 Key Concepts

### Module Lifecycle
```
insmod hello.ko
    ↓
hello_init() called
    ↓
Module running in kernel
    ↓
rmmod hello
    ↓
hello_exit() called
```

### Important Macros
- `module_init()`: Registers the initialization function
- `module_exit()`: Registers the cleanup function
- `__init`: Marks function code to be freed after initialization
- `__exit`: Marks function code that's only used during cleanup

## 🛠️ Build & Test

### Prerequisites
```bash
# Install kernel headers (if not already installed)
sudo apt-get install linux-headers-$(uname -r)
```

### Build the module
```bash
make
```

**Expected output:**
```
make -C /lib/modules/6.14.0-36-generic/build M=/path/to/01-hello-module modules
make[1]: Entering directory '/usr/src/linux-headers-6.14.0-36-generic'
  CC [M]  /path/to/01-hello-module/hello.o
  MODPOST /path/to/01-hello-module/Module.symvers
  CC [M]  /path/to/01-hello-module/hello.mod.o
  LD [M]  /path/to/01-hello-module/hello.ko
make[1]: Leaving directory '/usr/src/linux-headers-6.14.0-36-generic'
```

### Load the module
```bash
sudo insmod hello.ko
```

### Check kernel logs
```bash
dmesg | tail
```

**Expected output:**
```
[12345.678900] Hello: Module loaded successfully
[12345.678901] Hello: Kernel version 6.14.0-36-generic
```

### Unload the module
```bash
sudo rmmod hello
dmesg | tail
```

**Expected output:**
```
[12350.123456] Hello: Module unloaded, goodbye!
```

### Clean build artifacts
```bash
make clean
```

## 📝 Code Structure

```c
#include <linux/module.h>    // Core module support
#include <linux/kernel.h>    // Kernel logging (printk)
#include <linux/init.h>      // __init and __exit macros

static int __init hello_init(void) {
    // Called when module is loaded
    printk(KERN_INFO "Hello from kernel!\n");
    return 0;  // 0 = success
}

static void __exit hello_exit(void) {
    // Called when module is removed
    printk(KERN_INFO "Goodbye from kernel!\n");
}

module_init(hello_init);  // Register init function
module_exit(hello_exit);  // Register exit function

MODULE_LICENSE("GPL");    // Required: specifies license
```

## 🐛 Common Issues

### Issue: `make: *** No rule to make target 'modules'`
**Solution:** Install kernel headers
```bash
sudo apt-get install linux-headers-$(uname -r)
```

### Issue: `insmod: ERROR: could not insert module`
**Solution:** Check if you have root permissions
```bash
sudo insmod hello.ko
```

### Issue: Module already loaded
**Solution:** Unload first, then reload
```bash
sudo rmmod hello
sudo insmod hello.ko
```

## 📚 Key Takeaways

1. **Kernel modules** extend kernel functionality without recompiling
2. **printk()** is the kernel's equivalent of printf() 
3. **Log levels** (KERN_INFO, KERN_WARNING, KERN_ERR) control message priority
4. **Module metadata** (LICENSE, AUTHOR) is required for proper module loading
5. **Return 0** from init means success; negative values indicate errors

## 🔗 Next Steps

Move on to **02-char-device** to learn about:
- Creating device files (`/dev/hello`)
- Implementing file operations (open, read, write, release)
- User-space ↔ kernel-space communication
