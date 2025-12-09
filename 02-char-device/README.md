# 02 - Character Device Driver

A character device driver that implements read/write operations, demonstrating user-space ↔ kernel-space communication.

## 🎯 Learning Objectives

- Understanding character device registration
- Implementing file operations (open, read, write, release)
- User-space ↔ kernel-space data transfer
- Proper EOF (End-of-File) handling
- Device file creation and management

## 🔑 Key Concepts

### Character Device Architecture

```
User Space                  Kernel Space
----------                  ------------
   App
    ↓
 open("/dev/hello")  →  device_open()
    ↓                        ↓
 write(fd, "data")   →  device_write()
    ↓                        ↓
 read(fd, buf)       →  device_read()
    ↓                        ↓
 close(fd)           →  device_release()
```

### VFS Layer Flow

```
System Call (read/write)
    ↓
Virtual File System (VFS)
    ↓
file_operations structure
    ↓
Our driver functions (device_read/device_write)
    ↓
Kernel buffer (device_buffer)
```

### Major/Minor Numbers

- **Major Number**: Identifies the driver
- **Minor Number**: Identifies the specific device (we use 0)
- `/dev/hello` → `c 250 0` (example: major=250, minor=0)

## 🛠️ Build & Test

### Build the module
```bash
make
```

### Load module and create device file
```bash
make load
```

This will:
1. Load the module (`insmod chardev.ko`)
2. Get the assigned major number from `dmesg`
3. Create `/dev/hello` with the correct major number
4. Set permissions to allow read/write

### Test write operation
```bash
echo "Hello from userspace" > /dev/hello
```

### Test read operation
```bash
cat /dev/hello
```

**Expected output:**
```
Hello from userspace
```

### Check kernel logs
```bash
dmesg | tail -10
```

**Expected output:**
```
[12345.678900] chardev: Module loaded successfully
[12345.678901] chardev: Major number: 250
[12345.678902] chardev: Device opened
[12345.678903] chardev: Write requested: count=21
[12345.678904] chardev: Wrote 21 bytes: 'Hello from userspace'
[12345.678905] chardev: Device closed
[12345.678906] chardev: Device opened
[12345.678907] chardev: Read requested: count=131072, offset=0
[12345.678908] chardev: Read 21 bytes, new offset=21
[12345.678909] chardev: Read requested: count=131072, offset=21
[12345.678910] chardev: EOF reached
[12345.678911] chardev: Device closed
```

### Unload module
```bash
make unload
```

### Clean build artifacts
```bash
make clean
```

## 📝 Code Structure

### File Operations

```c
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,        // Called on open("/dev/hello")
    .release = device_release,  // Called on close(fd)
    .read = device_read,        // Called on read(fd, buf, count)
    .write = device_write,      // Called on write(fd, buf, count)
};
```

### Important Functions

#### `copy_to_user()`
Safely copies data from kernel space to user space.

```c
if (copy_to_user(user_buf, device_buffer, bytes)) {
    return -EFAULT;  // Failed to copy
}
```

**Why not `memcpy()`?**
- User pointers might be invalid
- Need to handle page faults
- Security checks required

#### `copy_from_user()`
Safely copies data from user space to kernel space.

```c
if (copy_from_user(device_buffer, user_buf, bytes)) {
    return -EFAULT;  // Failed to copy
}
```

### EOF Handling

Proper EOF handling is crucial for programs like `cat`:

```c
static ssize_t device_read(..., loff_t *offset) {
    // Return 0 when all data has been read
    if (*offset >= buffer_len) {
        return 0;  // EOF - tells cat to stop reading
    }
    
    // Calculate bytes to read
    bytes_to_read = buffer_len - *offset;
    
    // Copy data
    copy_to_user(...);
    
    // Update offset for next read
    *offset += bytes_to_read;
    
    return bytes_to_read;
}
```

**Without proper EOF:**
- `cat` would loop infinitely
- Programs wouldn't know when to stop reading

## 🔍 Deep Dive: How Commands Find the Driver

### Q: When I run `cat /dev/hello`, how does it reach my driver?

```
1. cat opens /dev/hello
   ↓
2. VFS looks up the inode for /dev/hello
   ↓
3. Inode contains major number (e.g., 250)
   ↓
4. Kernel looks up driver registered with major 250
   ↓
5. Finds our chardev module
   ↓
6. Gets file_operations structure (fops)
   ↓
7. Calls fops->open (our device_open)
   ↓
8. cat calls read() system call
   ↓
9. Kernel calls fops->read (our device_read)
   ↓
10. Our function copies data from device_buffer to cat's buffer
```

### Q: What's the difference between major and minor numbers?

**Major Number:**
- Identifies which **driver** handles the device
- Kernel uses this to route system calls to the correct driver
- Example: major 250 → our chardev driver

**Minor Number:**
- Identifies which **specific device** within the driver
- Useful when one driver manages multiple devices
- Example: `/dev/sda1` (minor 1), `/dev/sda2` (minor 2) - same driver, different partitions

## 🐛 Common Issues

### Issue: "Device or resource busy" when unloading
**Cause:** Device file is still open by another process

**Solution:**
```bash
# Find processes using the device
lsof /dev/hello

# Kill if necessary, then try again
sudo rmmod chardev
```

### Issue: "No such device" when accessing /dev/hello
**Cause:** Device file not created or wrong major number

**Solution:**
```bash
# Check if device file exists
ls -l /dev/hello

# Recreate it
make load
```

### Issue: `cat` hangs indefinitely
**Cause:** Missing EOF handling in device_read

**Solution:** Ensure device_read returns 0 when `*offset >= buffer_len`

## 📊 Testing Scenarios

### Test 1: Basic Read/Write
```bash
echo "test" > /dev/hello
cat /dev/hello  # Should print "test"
```

### Test 2: Multiple Writes
```bash
echo "first" > /dev/hello
echo "second" > /dev/hello
cat /dev/hello  # Should print "second" (overwrites)
```

### Test 3: Large Data
```bash
# Write 1KB of data
dd if=/dev/urandom bs=1024 count=1 | base64 > /dev/hello
cat /dev/hello
```

### Test 4: EOF Behavior
```bash
echo "test" > /dev/hello
# Read multiple times - should get data once, then EOF
cat /dev/hello
cat /dev/hello
```

## 📚 Key Takeaways

1. **Character devices** provide stream-oriented access (byte-by-byte)
2. **file_operations** structure connects system calls to driver functions
3. **copy_to_user/copy_from_user** safely transfer data across kernel boundary
4. **Proper EOF handling** is essential for correct behavior
5. **Major/minor numbers** identify drivers and specific devices
6. **Device files** in `/dev/` are the user-space interface to drivers

## 🔗 Next Steps

Move on to **03-ioctl-control** to learn about:
- Advanced device control using ioctl
- Parameter configuration (exposure, AWB, etc.)
- Bi-directional communication
- Status querying
