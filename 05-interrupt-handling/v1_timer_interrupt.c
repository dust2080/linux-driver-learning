/*
 * v1_timer_interrupt.c
 * 
 * Learning Goal: Understanding basic interrupt handler structure
 * 
 * This version uses a kernel timer to simulate hardware interrupts.
 * The timer fires every 2 seconds, triggering our interrupt handler.
 * 
 * Key Learning Points:
 * 1. How to set up a kernel timer
 * 2. Interrupt handler basic structure
 * 3. What can/cannot be done in interrupt context
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeff");
MODULE_DESCRIPTION("Module 05 v1: Basic interrupt handling with timer");
MODULE_VERSION("1.0");

/* ============================================
 * Global Variables
 * ============================================ */

static struct timer_list my_timer;
static int interrupt_count = 0;

/* ============================================
 * Interrupt Handler Simulation
 * ============================================ */

/*
 * This simulates what an interrupt handler does.
 * In a real driver, this would be called by hardware (e.g., GPIO interrupt).
 * 
 * IMPORTANT CONSTRAINTS:
 * - Cannot sleep
 * - Cannot use mutex/semaphore
 * - Must be fast
 * - Runs in atomic context
 */
static void simulate_interrupt_handler(void)
{
    interrupt_count++;
    
    pr_info("INTERRUPT #%d: Handler called (simulating hardware IRQ)\n", 
            interrupt_count);
    
    /*
     * In a real camera driver, we would:
     * 1. Read hardware status register
     * 2. Check which interrupt occurred
     * 3. Clear the interrupt flag
     * 4. Wake up waiting processes
     * 
     * For now, we just print a message.
     */
}

/* ============================================
 * Timer Callback Function
 * ============================================ */

/*
 * Timer callback function.
 * This is called when the timer expires.
 * 
 * We use this to simulate periodic hardware interrupts
 * (like a camera capturing frames every 2 seconds).
 */
static void timer_callback(struct timer_list *t)
{
    pr_info("TIMER: Expired, simulating interrupt...\n");
    
    /* Call our interrupt handler simulation */
    simulate_interrupt_handler();
    
    /* Re-arm the timer for 2 seconds later */
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
}

/* ============================================
 * Module Initialization
 * ============================================ */

static int __init interrupt_v1_init(void)
{
    pr_info("========================================\n");
    pr_info("Module 05 v1: Initializing\n");
    pr_info("========================================\n");
    
    /*
     * Initialize the timer.
     * timer_setup() associates our callback function with the timer.
     */
    timer_setup(&my_timer, timer_callback, 0);
    
    /*
     * Start the timer.
     * jiffies = current time (in kernel timer ticks)
     * msecs_to_jiffies(2000) = 2000ms = 2 seconds
     * 
     * So this timer will expire 2 seconds from now.
     */
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
    
    pr_info("Timer started: will fire every 2 seconds\n");
    pr_info("Watch 'dmesg -w' to see interrupts\n");
    pr_info("========================================\n");
    
    return 0;
}

/* ============================================
 * Module Cleanup
 * ============================================ */

static void __exit interrupt_v1_exit(void)
{
    /* Stop and delete the timer */
    del_timer(&my_timer);
    
    pr_info("========================================\n");
    pr_info("Module 05 v1: Removed\n");
    pr_info("Total interrupts handled: %d\n", interrupt_count);
    pr_info("========================================\n");
}

module_init(interrupt_v1_init);
module_exit(interrupt_v1_exit);
