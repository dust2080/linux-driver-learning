/*
 * hello.c - Simple "Hello World" kernel module
 * 
 * This is a minimal kernel module demonstrating:
 * - Module initialization and cleanup
 * - Kernel logging with printk
 * - Module metadata (license, author, description)
 * 
 * Build: make
 * Load: sudo insmod hello.ko
 * Check logs: dmesg | tail
 * Unload: sudo rmmod hello
 */

#include <linux/module.h>    /* Needed by all modules */
#include <linux/kernel.h>    /* Needed for KERN_INFO */
#include <linux/init.h>      /* Needed for __init and __exit macros */

/*
 * Module initialization function
 * Called when the module is loaded into the kernel (insmod)
 * Returns 0 on success, negative error code on failure
 */
static int __init hello_init(void)
{
    printk(KERN_INFO "Hello: Module loaded successfully\n");
    printk(KERN_INFO "Hello: Kernel version %s\n", UTS_RELEASE);
    return 0;  /* Success */
}

/*
 * Module cleanup function
 * Called when the module is removed from the kernel (rmmod)
 */
static void __exit hello_exit(void)
{
    printk(KERN_INFO "Hello: Module unloaded, goodbye!\n");
}

/*
 * Register module entry and exit points
 * These macros tell the kernel which functions to call
 * when loading and unloading the module
 */
module_init(hello_init);
module_exit(hello_exit);

/* Module metadata */
MODULE_LICENSE("GPL");                          /* License type (required) */
MODULE_AUTHOR("Jeff");                          /* Module author */
MODULE_DESCRIPTION("A simple hello world kernel module");  /* Description */
MODULE_VERSION("1.0");                          /* Version number */
