/*
 * v2_with_waitqueue.c
 * 
 * Learning Goal: Integrating interrupt handling with wait queue
 * 
 * This version combines:
 * - Module 04: poll() and wait queue mechanism
 * - Module 05: Interrupt handling
 * 
 * Flow:
 * 1. User calls poll() -> process sleeps on wait queue
 * 2. Timer fires (simulating hardware interrupt)
 * 3. Interrupt handler sets data_ready and calls wake_up()
 * 4. Process wakes up, poll() returns
 * 5. User calls read() to get data
 * 
 * This is how real camera drivers work!
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeff");
MODULE_DESCRIPTION("Module 05 v2: Interrupt + Wait Queue Integration");
MODULE_VERSION("2.0");

/* ============================================
 * Device Information
 * ============================================ */

#define DEVICE_NAME "camera"
#define CLASS_NAME "camera_class"

/* Image dimensions */
#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define BYTES_PER_PIXEL 2  // RAW12 stored as 16-bit
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT * BYTES_PER_PIXEL)

static int major_number;
static struct class *dev_class = NULL;
static struct device *dev_device = NULL;
static struct cdev my_cdev;

/* ============================================
 * Wait Queue and Data State
 * ============================================ */

/* 
 * Wait queue: where processes sleep when waiting for data
 * This is from Module 04
 */
static DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);

/*
 * Data ready flag: indicates if data is available
 * - false: no data, poll() will sleep
 * - true: data ready, poll() will return immediately
 */
static bool data_ready = false;

/*
 * Frame counter and buffer
 * Simulates camera frame data
 */
static int frame_count = 0;
static char *frame_buffer = NULL;  // Dynamically allocated

/* ============================================
 * Timer (Simulating Hardware Interrupt)
 * ============================================ */

static struct timer_list my_timer;

/* ============================================
 * Test Pattern Generation
 * ============================================ */

/*
 * Generate a simple test pattern for RAW image
 * Pattern: Gradient from top-left (dark) to bottom-right (bright)
 * Each frame is slightly different due to frame_count offset
 */
static void generate_test_pattern(void)
{
    int i, j;
    uint16_t *pixels = (uint16_t *)frame_buffer;
    
    for (i = 0; i < FRAME_HEIGHT; i++) {
        for (j = 0; j < FRAME_WIDTH; j++) {
            /* Simple gradient with frame_count offset for variation */
            /* RAW12: 12-bit values (0-4095) */
            pixels[i * FRAME_WIDTH + j] = ((i + j + frame_count * 10) * 16) % 4096;
        }
    }
}

/* ============================================
 * Interrupt Handler Simulation
 * ============================================ */

/*
 * This simulates what happens when hardware interrupt occurs.
 * 
 * In a real camera driver:
 * - This would be called by GPIO interrupt when frame is captured
 * - We would read frame buffer address from camera registers
 * - We would set up DMA transfer for frame data
 * 
 * Here we simulate by:
 * - Incrementing frame counter
 * - Generating test pattern image
 * - Setting data_ready flag
 * - Waking up waiting processes
 */
static void simulate_camera_interrupt(void)
{
    /* Simulate: Camera captured a new frame */
    frame_count++;
    
    /* Generate test pattern image */
    generate_test_pattern();
    
    pr_info("IRQ: Frame #%d ready (%dx%d, %d bytes)\n", 
            frame_count, FRAME_WIDTH, FRAME_HEIGHT, FRAME_SIZE);
    
    /*
     * KEY STEP 1: Set data ready flag
     * This is what poll() checks
     */
    data_ready = true;
    
    /*
     * KEY STEP 2: Wake up all processes waiting on the wait queue
     * This is the missing piece from Module 04!
     * 
     * wake_up_interruptible():
     * - Wakes up all processes sleeping on my_wait_queue
     * - "interruptible" means processes can be woken by signals
     * - After this, poll() will return to user space
     */
    wake_up_interruptible(&my_wait_queue);
    
    pr_info("IRQ: wake_up() called, processes should wake now\n");
}

/* ============================================
 * Timer Callback
 * ============================================ */

/*
 * Timer callback: simulates periodic camera frame capture
 * Real camera: GPIO interrupt every time a frame is ready
 * Our simulation: Timer interrupt every 2 seconds
 */
static void timer_callback(struct timer_list *t)
{
    pr_info("TIMER: Firing (simulating camera frame ready event)\n");
    
    /* Call our interrupt handler simulation */
    simulate_camera_interrupt();
    
    /* Re-arm timer for next "frame capture" */
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
}

/* ============================================
 * File Operations
 * ============================================ */

/*
 * open() - Called when user opens /dev/camera
 */
static int my_open(struct inode *inode, struct file *file)
{
    pr_info("DEVICE: opened by process %d\n", current->pid);
    return 0;
}

/*
 * release() - Called when user closes the device
 */
static int my_release(struct inode *inode, struct file *file)
{
    pr_info("DEVICE: closed by process %d\n", current->pid);
    return 0;
}

/*
 * read() - Called when user reads from device
 * 
 * Returns the frame data if available.
 */
static ssize_t my_read(struct file *file, char __user *buf,
                       size_t count, loff_t *ppos)
{
    size_t bytes_to_copy;
    int ret;
    
    pr_info("READ: called by process %d\n", current->pid);
    
    /* Check if data is ready */
    if (!data_ready) {
        pr_info("READ: No data available\n");
        return -EAGAIN;  /* Try again later */
    }
    
    /* Calculate how many bytes to copy */
    bytes_to_copy = min(count, (size_t)FRAME_SIZE);
    
    /* Copy data to user space */
    ret = copy_to_user(buf, frame_buffer, bytes_to_copy);
    if (ret) {
        pr_err("READ: Failed to copy %d bytes to user\n", ret);
        return -EFAULT;
    }
    
    /* Reset data ready flag (data has been consumed) */
    data_ready = false;
    
    pr_info("READ: Sent %zu bytes to user\n", bytes_to_copy);
    
    return bytes_to_copy;
}

/*
 * poll() - Called when user calls poll() or select()
 * 
 * This is from Module 04, but now we understand:
 * - poll_wait() registers us to the wait queue
 * - We check data_ready flag
 * - If no data, process will sleep
 * - wake_up() (called in interrupt handler) will wake us up
 */
static unsigned int my_poll(struct file *file, poll_table *wait)
{
    unsigned int mask = 0;
    
    pr_info("POLL: called by process %d\n", current->pid);
    
    /*
     * Register to wait queue.
     * This doesn't actually sleep yet - it just registers.
     * The actual sleep happens when we return 0 (no data ready).
     */
    poll_wait(file, &my_wait_queue, wait);
    
    /* Check if data is ready */
    if (data_ready) {
        /* Data is ready, return POLLIN to indicate readable */
        mask |= POLLIN | POLLRDNORM;
        pr_info("POLL: Data ready, returning POLLIN\n");
    } else {
        /* No data, process will sleep after we return 0 */
        pr_info("POLL: No data, process will sleep\n");
    }
    
    return mask;
}

/* File operations structure */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .poll = my_poll,
};

/* ============================================
 * Module Initialization
 * ============================================ */

static int __init interrupt_v2_init(void)
{
    dev_t dev;
    int ret;
    
    pr_info("========================================\n");
    pr_info("Module 05 v2: Initializing\n");
    pr_info("========================================\n");
    
    /* 1. Allocate device number */
    ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("Failed to allocate device number\n");
        return ret;
    }
    major_number = MAJOR(dev);
    pr_info("Allocated major number: %d\n", major_number);
    
    /* 2. Initialize cdev */
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    
    /* 3. Add cdev to kernel */
    ret = cdev_add(&my_cdev, dev, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev, 1);
        pr_err("Failed to add cdev\n");
        return ret;
    }
    
    /* 4. Create device class */
    dev_class = class_create(CLASS_NAME);
    if (IS_ERR(dev_class)) {
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev, 1);
        pr_err("Failed to create class\n");
        return PTR_ERR(dev_class);
    }
    
    /* 5. Create device node */
    dev_device = device_create(dev_class, NULL, dev, NULL, DEVICE_NAME);
    if (IS_ERR(dev_device)) {
        class_destroy(dev_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev, 1);
        pr_err("Failed to create device\n");
        return PTR_ERR(dev_device);
    }
    
    pr_info("Device created: /dev/%s\n", DEVICE_NAME);
    
    /* 6. Allocate frame buffer */
    frame_buffer = kmalloc(FRAME_SIZE, GFP_KERNEL);
    if (!frame_buffer) {
        device_destroy(dev_class, dev);
        class_destroy(dev_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev, 1);
        pr_err("Failed to allocate frame buffer\n");
        return -ENOMEM;
    }
    pr_info("Frame buffer allocated: %d bytes\n", FRAME_SIZE);
    
    /* 7. Start timer (simulating periodic camera interrupts) */
    timer_setup(&my_timer, timer_callback, 0);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
    
    pr_info("Timer started: simulating camera frames every 2 seconds\n");
    pr_info("========================================\n");
    pr_info("Ready! Test with: ./interrupt_test\n");
    pr_info("========================================\n");
    
    return 0;
}

/* ============================================
 * Module Cleanup
 * ============================================ */

static void __exit interrupt_v2_exit(void)
{
    dev_t dev = MKDEV(major_number, 0);
    
    /* Stop timer */
    del_timer(&my_timer);
    
    /* Free frame buffer */
    if (frame_buffer) {
        kfree(frame_buffer);
    }
    
    /* Remove device */
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, 1);
    
    pr_info("========================================\n");
    pr_info("Module 05 v2: Removed\n");
    pr_info("Total frames captured: %d\n", frame_count);
    pr_info("========================================\n");
}

module_init(interrupt_v2_init);
module_exit(interrupt_v2_exit);
