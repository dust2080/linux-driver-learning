/*
 * poll_driver.c - Character device driver with poll/select support
 * 
 * This driver demonstrates:
 * - poll() and select() system call implementation
 * - Wait queues for asynchronous I/O
 * - Non-blocking read operations
 * - Event notification mechanism
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>

#define DEVICE_NAME "poll_device"
#define CLASS_NAME "poll_class"
#define BUFFER_SIZE 1024

struct poll_device {
    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    
    char *buffer;
    size_t buffer_size;
    struct mutex mutex;
    
    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;
    
    bool data_available;
};

static struct poll_device *poll_dev;

static int poll_open(struct inode *inode, struct file *filp)
{
    pr_info("poll_driver: Device opened\n");
    filp->private_data = poll_dev;
    return 0;
}

static int poll_release(struct inode *inode, struct file *filp)
{
    pr_info("poll_driver: Device closed\n");
    return 0;
}

static ssize_t poll_read(struct file *filp, char __user *buf,
                         size_t count, loff_t *f_pos)
{
    struct poll_device *dev = filp->private_data;
    ssize_t bytes_read = 0;
    int ret;
    
    pr_info("poll_driver: read() called with count=%zu\n", count);
    
    if (mutex_lock_interruptible(&dev->mutex))
        return -ERESTARTSYS;
    
    if (!dev->data_available && (filp->f_flags & O_NONBLOCK)) {
        mutex_unlock(&dev->mutex);
        return -EAGAIN;
    }
    
    while (!dev->data_available) {
        mutex_unlock(&dev->mutex);
        pr_info("poll_driver: No data, going to sleep...\n");
        
        ret = wait_event_interruptible(dev->read_queue, dev->data_available);
        if (ret)
            return -ERESTARTSYS;
        
        if (mutex_lock_interruptible(&dev->mutex))
            return -ERESTARTSYS;
    }
    
    bytes_read = min(count, dev->buffer_size);
    
    if (copy_to_user(buf, dev->buffer, bytes_read)) {
        mutex_unlock(&dev->mutex);
        return -EFAULT;
    }
    
    pr_info("poll_driver: Read %zd bytes\n", bytes_read);
    
    dev->buffer_size = 0;
    dev->data_available = false;
    
    mutex_unlock(&dev->mutex);
    wake_up_interruptible(&dev->write_queue);
    
    return bytes_read;
}

static ssize_t poll_write(struct file *filp, const char __user *buf,
                          size_t count, loff_t *f_pos)
{
    struct poll_device *dev = filp->private_data;
    ssize_t bytes_written;
    
    pr_info("poll_driver: write() called with count=%zu\n", count);
    
    if (mutex_lock_interruptible(&dev->mutex))
        return -ERESTARTSYS;
    
    bytes_written = min(count, (size_t)BUFFER_SIZE);
    
    if (copy_from_user(dev->buffer, buf, bytes_written)) {
        mutex_unlock(&dev->mutex);
        return -EFAULT;
    }
    
    dev->buffer_size = bytes_written;
    dev->data_available = true;
    
    pr_info("poll_driver: Wrote %zd bytes\n", bytes_written);
    
    mutex_unlock(&dev->mutex);
    wake_up_interruptible(&dev->read_queue);
    
    return bytes_written;
}

static __poll_t poll_poll(struct file *filp, struct poll_table_struct *wait)
{
    struct poll_device *dev = filp->private_data;
    __poll_t mask = 0;
    
    pr_info("poll_driver: poll() called\n");
    
    poll_wait(filp, &dev->read_queue, wait);
    poll_wait(filp, &dev->write_queue, wait);
    
    mutex_lock(&dev->mutex);
    
    if (dev->data_available) {
        mask |= POLLIN | POLLRDNORM;
        pr_info("poll_driver: Data available - returning POLLIN\n");
    }
    
    mask |= POLLOUT | POLLWRNORM;
    
    mutex_unlock(&dev->mutex);
    
    return mask;
}

static const struct file_operations poll_fops = {
    .owner = THIS_MODULE,
    .open = poll_open,
    .release = poll_release,
    .read = poll_read,
    .write = poll_write,
    .poll = poll_poll,
};

static int __init poll_driver_init(void)
{
    int ret;
    
    pr_info("poll_driver: Initializing driver\n");
    
    poll_dev = kzalloc(sizeof(struct poll_device), GFP_KERNEL);
    if (!poll_dev)
        return -ENOMEM;
    
    poll_dev->buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!poll_dev->buffer) {
        ret = -ENOMEM;
        goto fail_buffer;
    }
    
    mutex_init(&poll_dev->mutex);
    init_waitqueue_head(&poll_dev->read_queue);
    init_waitqueue_head(&poll_dev->write_queue);
    
    poll_dev->data_available = false;
    poll_dev->buffer_size = 0;
    
    ret = alloc_chrdev_region(&poll_dev->dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("poll_driver: Failed to allocate device number\n");
        goto fail_alloc_chrdev;
    }
    
    pr_info("poll_driver: Major: %d, Minor: %d\n",
            MAJOR(poll_dev->dev_num), MINOR(poll_dev->dev_num));
    
    cdev_init(&poll_dev->cdev, &poll_fops);
    poll_dev->cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&poll_dev->cdev, poll_dev->dev_num, 1);
    if (ret < 0) {
        pr_err("poll_driver: Failed to add cdev\n");
        goto fail_cdev_add;
    }
    
    poll_dev->class = class_create(CLASS_NAME);
    if (IS_ERR(poll_dev->class)) {
        ret = PTR_ERR(poll_dev->class);
        pr_err("poll_driver: Failed to create class\n");
        goto fail_class_create;
    }
    
    poll_dev->device = device_create(poll_dev->class, NULL,
                                     poll_dev->dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(poll_dev->device)) {
        ret = PTR_ERR(poll_dev->device);
        pr_err("poll_driver: Failed to create device\n");
        goto fail_device_create;
    }
    
    pr_info("poll_driver: Driver initialized successfully\n");
    pr_info("poll_driver: Device created at /dev/%s\n", DEVICE_NAME);
    
    return 0;

fail_device_create:
    class_destroy(poll_dev->class);
fail_class_create:
    cdev_del(&poll_dev->cdev);
fail_cdev_add:
    unregister_chrdev_region(poll_dev->dev_num, 1);
fail_alloc_chrdev:
    kfree(poll_dev->buffer);
fail_buffer:
    kfree(poll_dev);
    return ret;
}

static void __exit poll_driver_exit(void)
{
    pr_info("poll_driver: Cleaning up driver\n");
    
    device_destroy(poll_dev->class, poll_dev->dev_num);
    class_destroy(poll_dev->class);
    cdev_del(&poll_dev->cdev);
    unregister_chrdev_region(poll_dev->dev_num, 1);
    kfree(poll_dev->buffer);
    kfree(poll_dev);
    
    pr_info("poll_driver: Driver cleanup complete\n");
}

module_init(poll_driver_init);
module_exit(poll_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeff Chang");
MODULE_DESCRIPTION("Character device driver with poll/select support");
MODULE_VERSION("1.0");
