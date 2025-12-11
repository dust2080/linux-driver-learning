/*
 * ioctl_driver.c - Character device driver with ioctl support
 * 
 * This driver demonstrates ioctl usage for device parameter control.
 * It simulates an ISP (Image Signal Processor) device with configurable
 * parameters like gain, exposure, and white balance.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "ioctl_cmd.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeff");
MODULE_DESCRIPTION("ioctl control driver for ISP-like device");
MODULE_VERSION("1.0");

/* Device information */
#define DEVICE_NAME "ioctl_dev"
#define CLASS_NAME  "ioctl_class"

/* Default device parameters */
#define DEFAULT_GAIN        50      /* Default gain: 50% */
#define DEFAULT_EXPOSURE    33      /* Default exposure: 33ms (30fps) */
#define DEFAULT_WB_TEMP     5500    /* Default WB: daylight 5500K */

/* Global variables */
static int major_number;
static struct class *device_class = NULL;
static struct device *char_device = NULL;
static struct cdev ioctl_cdev;

/*
 * Device state structure
 * This structure holds the current state of our virtual device
 */
struct device_state {
    struct device_params params;    /* Current parameters */
    unsigned int is_streaming;      /* Streaming state */
    unsigned int frame_count;       /* Frame counter */
    struct mutex lock;              /* Mutex for thread safety */
};

static struct device_state *dev_state;

/*
 * Initialize device to default parameters
 */
static void reset_device(struct device_state *state)
{
    state->params.gain = DEFAULT_GAIN;
    state->params.exposure = DEFAULT_EXPOSURE;
    state->params.wb_temp = DEFAULT_WB_TEMP;
    state->is_streaming = 0;
    state->frame_count = 0;
    
    pr_info("ioctl_dev: Device reset to defaults (gain=%u, exposure=%u, wb_temp=%u)\n",
            state->params.gain, state->params.exposure, state->params.wb_temp);
}

/*
 * Validate device parameters
 * Returns 0 if valid, -EINVAL if invalid
 */
static int validate_params(const struct device_params *params)
{
    if (params->gain > 100) {
        pr_err("ioctl_dev: Invalid gain %u (must be 0-100)\n", params->gain);
        return -EINVAL;
    }
    
    if (params->exposure < 1 || params->exposure > 1000) {
        pr_err("ioctl_dev: Invalid exposure %u (must be 1-1000 ms)\n", params->exposure);
        return -EINVAL;
    }
    
    if (params->wb_temp < 2000 || params->wb_temp > 10000) {
        pr_err("ioctl_dev: Invalid white balance %u (must be 2000-10000 K)\n", params->wb_temp);
        return -EINVAL;
    }
    
    return 0;
}

/*
 * Device open operation
 */
static int device_open(struct inode *inode, struct file *file)
{
    pr_info("ioctl_dev: Device opened\n");
    return 0;
}

/*
 * Device release operation
 */
static int device_release(struct inode *inode, struct file *file)
{
    pr_info("ioctl_dev: Device closed\n");
    return 0;
}

/*
 * ioctl operation - the heart of this driver
 * 
 * This function handles all ioctl commands from user space.
 * It's where the actual device control logic is implemented.
 */
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct device_params params;
    struct device_status status;
    
    /* Lock the device state to ensure thread safety */
    mutex_lock(&dev_state->lock);
    
    switch (cmd) {
        case IOCTL_RESET:
            pr_info("ioctl_dev: IOCTL_RESET called\n");
            reset_device(dev_state);
            break;
            
        case IOCTL_SET_PARAMS:
            pr_info("ioctl_dev: IOCTL_SET_PARAMS called\n");
            
            /* Copy parameters from user space */
            if (copy_from_user(&params, (struct device_params __user *)arg, 
                              sizeof(struct device_params))) {
                pr_err("ioctl_dev: Failed to copy parameters from user\n");
                ret = -EFAULT;
                break;
            }
            
            /* Validate parameters */
            ret = validate_params(&params);
            if (ret) {
                break;
            }
            
            /* Apply parameters */
            dev_state->params = params;
            pr_info("ioctl_dev: Parameters updated (gain=%u, exposure=%u, wb_temp=%u)\n",
                    params.gain, params.exposure, params.wb_temp);
            break;
            
        case IOCTL_GET_PARAMS:
            pr_info("ioctl_dev: IOCTL_GET_PARAMS called\n");
            
            /* Copy current parameters to user space */
            if (copy_to_user((struct device_params __user *)arg, &dev_state->params,
                            sizeof(struct device_params))) {
                pr_err("ioctl_dev: Failed to copy parameters to user\n");
                ret = -EFAULT;
            }
            break;
            
        case IOCTL_GET_STATUS:
            pr_info("ioctl_dev: IOCTL_GET_STATUS called\n");
            
            /* Prepare status information */
            status.is_streaming = dev_state->is_streaming;
            status.frame_count = dev_state->frame_count;
            status.params = dev_state->params;
            
            /* Copy status to user space */
            if (copy_to_user((struct device_status __user *)arg, &status,
                            sizeof(struct device_status))) {
                pr_err("ioctl_dev: Failed to copy status to user\n");
                ret = -EFAULT;
            }
            break;
            
        case IOCTL_START_STREAM:
            pr_info("ioctl_dev: IOCTL_START_STREAM called\n");
            
            if (dev_state->is_streaming) {
                pr_warn("ioctl_dev: Device is already streaming\n");
                ret = -EBUSY;
            } else {
                dev_state->is_streaming = 1;
                dev_state->frame_count = 0;
                pr_info("ioctl_dev: Streaming started\n");
            }
            break;
            
        case IOCTL_STOP_STREAM:
            pr_info("ioctl_dev: IOCTL_STOP_STREAM called\n");
            
            if (!dev_state->is_streaming) {
                pr_warn("ioctl_dev: Device is not streaming\n");
                ret = -EINVAL;
            } else {
                dev_state->is_streaming = 0;
                pr_info("ioctl_dev: Streaming stopped (total frames: %u)\n",
                        dev_state->frame_count);
            }
            break;
            
        default:
            pr_err("ioctl_dev: Invalid ioctl command: 0x%x\n", cmd);
            ret = -ENOTTY;  /* Inappropriate ioctl for device */
            break;
    }
    
    mutex_unlock(&dev_state->lock);
    return ret;
}

/* File operations structure */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,  /* ioctl handler */
};

/*
 * Module initialization
 */
static int __init ioctl_driver_init(void)
{
    dev_t dev_num;
    int ret;
    
    pr_info("ioctl_dev: Initializing driver\n");
    
    /* Allocate device state */
    dev_state = kzalloc(sizeof(struct device_state), GFP_KERNEL);
    if (!dev_state) {
        pr_err("ioctl_dev: Failed to allocate device state\n");
        return -ENOMEM;
    }
    
    /* Initialize device state */
    mutex_init(&dev_state->lock);
    reset_device(dev_state);
    
    /* Allocate major number dynamically */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("ioctl_dev: Failed to allocate major number\n");
        kfree(dev_state);
        return ret;
    }
    major_number = MAJOR(dev_num);
    pr_info("ioctl_dev: Registered with major number %d\n", major_number);
    
    /* Initialize cdev structure */
    cdev_init(&ioctl_cdev, &fops);
    ioctl_cdev.owner = THIS_MODULE;
    
    /* Add cdev to system */
    ret = cdev_add(&ioctl_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("ioctl_dev: Failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        kfree(dev_state);
        return ret;
    }
    
    /* Create device class */
    device_class = class_create(CLASS_NAME);
    if (IS_ERR(device_class)) {
        pr_err("ioctl_dev: Failed to create device class\n");
        cdev_del(&ioctl_cdev);
        unregister_chrdev_region(dev_num, 1);
        kfree(dev_state);
        return PTR_ERR(device_class);
    }
    
    /* Create device file */
    char_device = device_create(device_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(char_device)) {
        pr_err("ioctl_dev: Failed to create device\n");
        class_destroy(device_class);
        cdev_del(&ioctl_cdev);
        unregister_chrdev_region(dev_num, 1);
        kfree(dev_state);
        return PTR_ERR(char_device);
    }
    
    pr_info("ioctl_dev: Driver initialized successfully\n");
    pr_info("ioctl_dev: Device created at /dev/%s\n", DEVICE_NAME);
    return 0;
}

/*
 * Module cleanup
 */
static void __exit ioctl_driver_exit(void)
{
    dev_t dev_num = MKDEV(major_number, 0);
    
    pr_info("ioctl_dev: Cleaning up driver\n");
    
    /* Remove device file */
    device_destroy(device_class, dev_num);
    
    /* Remove device class */
    class_destroy(device_class);
    
    /* Remove cdev */
    cdev_del(&ioctl_cdev);
    
    /* Unregister device number */
    unregister_chrdev_region(dev_num, 1);
    
    /* Free device state */
    kfree(dev_state);
    
    pr_info("ioctl_dev: Driver removed\n");
}

module_init(ioctl_driver_init);
module_exit(ioctl_driver_exit);
