# Module 04: Poll/Select - Asynchronous I/O

## Overview

This module demonstrates asynchronous I/O using `poll()` and `select()` system calls with wait queues.

## Quick Start
```bash
# Build
make all

# Run full test
make fulltest

# Manual workflow
make install    # Load driver
make test       # Run tests
make uninstall  # Unload driver
```

## Learning Objectives

1. **poll() and select()** - Monitor multiple file descriptors
2. **Wait queues** - Kernel event notification mechanism
3. **Non-blocking I/O** - Using O_NONBLOCK flag
4. **Event-driven programming** - Building responsive applications

## Key Concepts

### poll() System Call
```c
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

Returns when:
- FD becomes ready (POLLIN/POLLOUT)
- Timeout expires
- Signal received

### Wait Queue Mechanism
```
User Space              Kernel Space
----------              ------------

poll()  ─────────►  poll_poll()
  │                     │
  │                     ├─► poll_wait() (register)
  │                     └─► Check status
  │
  ▼ (no data)
[SLEEP]
  ▲
  │
write() ─────────►  write_handler()
                        │
                        └─► wake_up()
[WAKE]
  │
  ▼
poll() returns
```

## Implementation

### Driver (poll_driver.c)

Key components:
- `poll_poll()` - Implement poll support
- `poll_wait()` - Register for notification
- `wake_up_interruptible()` - Notify waiting processes

### Tests (poll_test.c)

6 test cases:
1. poll() timeout (no data)
2. poll() with data available
3. poll() blocking until data arrives
4. select() system call
5. Non-blocking read (O_NONBLOCK)
6. Multiple file descriptors

## Expected Output
```
╔════════════════════════════════════════════════════╗
║   Poll/Select Driver - Test Suite                 ║
╚════════════════════════════════════════════════════╝

========================================
Test: Basic poll() with timeout (no data)
========================================
✓ poll() timed out as expected

[... more tests ...]

Test Summary
========================================
Passed: 12
Total: 12
```

## Debugging
```bash
# Watch kernel logs
sudo dmesg -w

# Trace system calls
strace -e poll,select,read,write ./poll_test

# Check device
ls -l /dev/poll_device
```

## Key Takeaways

1. **poll() doesn't block forever** - Has timeout parameter
2. **Wait queues are the bridge** - Between user and kernel space
3. **Non-blocking I/O** - Returns EAGAIN instead of sleeping
4. **Multiple FDs efficiently** - Single poll() call monitors all

## Real-World Applications

- Network servers (multiple client sockets)
- GUI applications (keyboard + mouse)
- Embedded systems (multiple sensors)
- Event-driven systems

## Next Steps

- Module 05: Interrupt handling
- Module 06: DMA operations
- Module 07: Platform devices

## References

- [poll(2) man page](https://man7.org/linux/man-pages/man2/poll.2.html)
- [LDD3 Chapter 6](https://lwn.net/Kernel/LDD3/)

---

**Author:** Jeff Chang  
**Date:** December 12, 2025
