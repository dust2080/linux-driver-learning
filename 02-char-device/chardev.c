/*
 * chardev.c - Character device driver with read/write operations
 * 
 * This module demonstrates:
 * - Character device registration
 * - File operations (open, read, write, release)
 * - User-space ↔ kernel-space data transfer
 * - Proper EOF handling
 * 
 * Device file: /dev/hello (created with mknod or udev)
 * Major number: dynamically allocated
 * 
 * Usage:
 *   echo "hello" > /dev/hello    # Write to device
 *   cat /dev/hello                # Read from device
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>           /* file_operations, register_chrdev */
#include <linux/uaccess.h>      /* copy_to_user, copy_from_user */
#include <linux/slab.h>         /* kmalloc, kfree */

#define DEVICE_NAME "hello"     /* Device name as it appears in /proc/devices */
#define BUF_SIZE 1024          /* Maximum buffer size */

/* Global variables */
static int major_num;           /* Major number assigned to our device */
static char *device_buffer;     /* Kernel buffer to store data */
static size_t buffer_len = 0;   /* Current length of data in buffer */

/*
 * device_open - Called when a process opens the device file
 * @inode: Inode structure (contains major/minor numbers)
 * @file: File structure (contains file operations)
 * 
 * This is called when userspace does: open("/dev/hello", ...)
 * Return: 0 on success, negative error code on failure
 */
static int device_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "chardev: Device opened\n");
    
    /*
     * We could track the number of times the device is opened here,
     * or perform any necessary initialization for this file handle
     */
    
    return 0;
}

/*
 * device_release - Called when a process closes the device file
 * @inode: Inode structure
 * @file: File structure
 * 
 * This is called when userspace does: close(fd)
 * Return: 0 on success
 */
static int device_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "chardev: Device closed\n");
    
    /*
     * We could perform any cleanup operations here,
     * such as flushing buffers or releasing resources
     */
    
    return 0;
}

/*
 * device_read - Called when a process reads from the device
 * @file: File structure
 * @user_buf: User-space buffer to write data to
 * @count: Number of bytes requested
 * @offset: Current file position
 * 
 * This is called when userspace does: read(fd, buf, count)
 * 
 * IMPORTANT: Must handle EOF correctly by returning 0 when no more data
 * Return: Number of bytes actually read, 0 for EOF, negative for error
 */
static ssize_t device_read(struct file *file, char __user *user_buf,
                          size_t count, loff_t *offset)
{
    size_t bytes_to_read;
    
    printk(KERN_INFO "chardev: Read requested: count=%zu, offset=%lld\n", 
           count, *offset);
    
    /* Check if we've already read all the data (EOF) */
    if (*offset >= buffer_len) {
        printk(KERN_INFO "chardev: EOF reached\n");
        return 0;  /* EOF */
    }
    
    /* Calculate how many bytes we can actually read */
    bytes_to_read = buffer_len - *offset;
    if (bytes_to_read > count) {
        bytes_to_read = count;
    }
    
    /* Copy data from kernel space to user space */
    if (copy_to_user(user_buf, device_buffer + *offset, bytes_to_read)) {
        printk(KERN_ERR "chardev: Failed to copy data to user space\n");
        return -EFAULT;  /* Bad address */
    }
    
    /* Update file position */
    *offset += bytes_to_read;
    
    printk(KERN_INFO "chardev: Read %zu bytes, new offset=%lld\n", 
           bytes_to_read, *offset);
    
    return bytes_to_read;
}

/*
 * device_write - Called when a process writes to the device
 * @file: File structure
 * @user_buf: User-space buffer containing data to write
 * @count: Number of bytes to write
 * @offset: Current file position (not used in this simple implementation)
 * 
 * This is called when userspace does: write(fd, buf, count)
 * 
 * Return: Number of bytes actually written, negative for error
 */
static ssize_t device_write(struct file *file, const char __user *user_buf,
                           size_t count, loff_t *offset)
{
    size_t bytes_to_write;
    
    printk(KERN_INFO "chardev: Write requested: count=%zu\n", count);
    
    /* Limit write size to our buffer capacity */
    bytes_to_write = (count < BUF_SIZE) ? count : BUF_SIZE;
    
    /* Clear existing buffer content */
    memset(device_buffer, 0, BUF_SIZE);
    
    /* Copy data from user space to kernel space */
    if (copy_from_user(device_buffer, user_buf, bytes_to_write)) {
        printk(KERN_ERR "chardev: Failed to copy data from user space\n");
        return -EFAULT;  /* Bad address */
    }
    
    buffer_len = bytes_to_write;
    
    printk(KERN_INFO "chardev: Wrote %zu bytes: '%.*s'\n", 
           bytes_to_write, (int)bytes_to_write, device_buffer);
    
    return bytes_to_write;
}

/*
 * File operations structure
 * This tells the kernel which functions to call for each file operation
 */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

/*
 * Module initialization
 * Called when the module is loaded (insmod)
 */
static int __init chardev_init(void)
{
    /* Allocate kernel buffer for data storage */
    device_buffer = kmalloc(BUF_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        printk(KERN_ERR "chardev: Failed to allocate buffer\n");
        return -ENOMEM;
    }
    memset(device_buffer, 0, BUF_SIZE);
    
    /*
     * Register character device
     * major_num = 0 means kernel will automatically assign a major number
     */
    major_num = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_num < 0) {
        printk(KERN_ERR "chardev: Failed to register device (error %d)\n", 
               major_num);
        kfree(device_buffer);
        return major_num;
    }
    
    printk(KERN_INFO "chardev: Module loaded successfully\n");
    printk(KERN_INFO "chardev: Major number: %d\n", major_num);
    printk(KERN_INFO "chardev: Create device file with:\n");
    printk(KERN_INFO "chardev:   sudo mknod /dev/%s c %d 0\n", 
           DEVICE_NAME, major_num);
    printk(KERN_INFO "chardev:   sudo chmod 666 /dev/%s\n", DEVICE_NAME);
    
    return 0;
}

/*
 * Module cleanup
 * Called when the module is removed (rmmod)
 */
static void __exit chardev_exit(void)
{
    /* Unregister the device */
    unregister_chrdev(major_num, DEVICE_NAME);
    
    /* Free allocated memory */
    kfree(device_buffer);
    
    printk(KERN_INFO "chardev: Module unloaded\n");
    printk(KERN_INFO "chardev: Remember to remove device file:\n");
    printk(KERN_INFO "chardev:   sudo rm /dev/%s\n", DEVICE_NAME);
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeff");
MODULE_DESCRIPTION("Character device driver with read/write operations");
MODULE_VERSION("1.0");
