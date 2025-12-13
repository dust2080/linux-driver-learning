# Module 05: Interrupt Handling

## What I'm Learning

Understanding how hardware interrupts work in Linux kernel drivers and how they integrate with the wait queue mechanism from Module 04.

## Learning Goals

1. **Interrupt basics**: Why we need interrupts vs polling
2. **Handler constraints**: What can/cannot be done in interrupt context
3. **Wait queue integration**: How interrupts wake up sleeping processes
4. **Real-world application**: How camera drivers use interrupts

## Why This Module?

After Module 04, I realized the wait queue mechanism needs something to call `wake_up()`. This module answers that question: **hardware interrupts**.

## Files

- `v1_timer_interrupt.c` - Basic interrupt handler using kernel timer
- `v2_with_waitqueue.c` - Integration with wait queue (complete async I/O)
- `interrupt_test.c` - User space test program
- `Makefile` - Build configuration
- `learning_notes.md` - What I learned, mistakes I made

## Key Concepts

### Interrupt Context vs Process Context
```
Process Context:
- Normal kernel code
- Can sleep
- Can use mutex
- Examples: read(), write(), ioctl()

Interrupt Context:
- Runs when interrupt occurs
- CANNOT sleep
- CANNOT use mutex
- Must be fast
- Example: interrupt handler
```

### Integration with Module 04
```
Module 04: Provides wait mechanism
    ↓
poll_wait() - registers to wait queue
    ↓
Process sleeps

Module 05: Provides wake mechanism
    ↓
Interrupt occurs
    ↓
Handler calls wake_up()
    ↓
Process wakes up
```

## Testing Approach

Since I don't have real camera hardware, I use kernel timer to simulate periodic interrupts (like frame capture events).
```
Timer (every 2s) = Camera capturing frame
    ↓
Timer callback = Interrupt handler
    ↓
wake_up() = Notify waiting process
```

## Build and Test
```bash
# Build
make

# Load driver
sudo insmod v2_with_waitqueue.ko

# Check device
ls -l /dev/interrupt_dev

# Run test (in another terminal)
./interrupt_test

# Watch kernel log (in another terminal)
dmesg -w

# Unload
sudo rmmod v2_with_waitqueue
```

## Expected Output

**Terminal 1 (test program):**
```
=== Interrupt Test Program ===
Device opened. Waiting for interrupts...

[1] Calling poll()...
[1] poll() returned: Data ready!
[1] Read: Frame #1 captured

[2] Calling poll()...
[2] poll() returned: Data ready!
[2] Read: Frame #2 captured
```

**Terminal 2 (kernel log):**
```
[...] === Interrupt Module v2: Init ===
[...] Device created: /dev/interrupt_dev
[...] poll(): Called
[...] poll(): No data, process will sleep
[...] IRQ: Frame #1 ready        <- Timer triggered
[...] poll(): Data ready, returning POLLIN
[...] read(): Sent 20 bytes
```

## What's Next

After understanding interrupt basics, I'll:
1. Add top/bottom half separation (v3)
2. Integrate with my ISP Pipeline project
3. Update resume and contact Google recruiter

## Resources

- Linux Device Drivers, 3rd Edition - Chapter 10
- Understanding the Linux Kernel - Chapter 4
- LWN.net articles on interrupt handling
