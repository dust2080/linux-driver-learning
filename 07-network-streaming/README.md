# Module 07: Network Streaming & ISP Integration

## Overview
Complete end-to-end camera system demonstrating driver-to-ISP integration via TCP/IP network streaming. This module combines:
- Linux driver (Module 05) generating RAW image frames
- Network transmission using TCP sockets
- Remote ISP Pipeline processing with CUDA acceleration

## Architecture
```
┌─────────────────────────────────────────────────────────────┐
│                    Ubuntu VM (ARM64)                        │
│                                                             │
│  ┌──────────────┐  poll()   ┌─────────────────┐           │
│  │ /dev/camera  │ ────────→ │ frame_streamer  │           │
│  │ (Module 05)  │  read()   │   (Server)      │           │
│  │              │ ────────→ │                 │           │
│  └──────────────┘           └─────────────────┘           │
│   640x480 RAW                      │                       │
│   2 sec interval                   │ TCP Socket            │
│                                    │ Port 8080             │
└────────────────────────────────────┼───────────────────────┘
                                     │
                            Internet │
                                     │
┌────────────────────────────────────┼───────────────────────┐
│                                    ▼                       │
│                    macOS (Apple Silicon)                   │
│                                                            │
│  ┌─────────────────┐         ┌──────────────────┐        │
│  │ frame_receiver  │ recv()  │  ISP Pipeline    │        │
│  │   (Client)      │ ──────→ │  - BLC           │        │
│  │                 │         │  - Demosaic      │        │
│  └─────────────────┘         │  - AWB           │        │
│         │                    │  - Gamma         │        │
│         │ save               │  - Denoise       │        │
│         ▼                    │  - Sharpen       │        │
│  frame_001.raw               └──────────────────┘        │
│  frame_002.raw                       │                   │
│  ...                                 ▼                   │
│                               output_001.png             │
│                               output_002.png             │
└──────────────────────────────────────────────────────────┘
```

## Features

### Phase 1: Network Foundation ✅
- TCP socket communication (reliable transmission)
- Client-server architecture
- Connection management

### Phase 2: Real Image Streaming ✅
- 640×480 RAW12 frames (614,400 bytes/frame)
- Dynamic test pattern generation (each frame unique)
- poll()-based blocking I/O (CPU-efficient)
- Automatic 5-frame transmission limit

### Phase 3: ISP Integration ✅
- Seamless integration with [ISP Pipeline](https://github.com/dust2080/ISP_Pipeline)
- Automatic RAW to PNG conversion
- 7-stage processing pipeline (1930x CUDA speedup on GPU)

## Components

### frame_streamer.c (VM Server)
**Purpose:** Bridge between kernel driver and network

**Flow:**
1. Opens `/dev/camera` character device
2. Creates TCP server socket (port 8080)
3. Waits for client connection
4. **Main loop (5 iterations):**
   - Calls `poll()` to wait for frame ready (blocks until interrupt)
   - Reads 614,400 bytes from driver
   - Sends complete frame via TCP
5. Closes connection after 5 frames

**Key Implementation:**
- Uses `poll()` for efficient I/O (process sleeps until driver wake-up)
- Ensures complete frame transmission with loop
- Clean shutdown mechanism

### frame_receiver.cpp (macOS Client)
**Purpose:** Receive frames and process with ISP

**Flow:**
1. Connects to VM server (192.168.64.2:8080)
2. **Per frame:**
   - Receives complete 614,400-byte frame
   - Saves as `frame_XXX.raw`
   - Invokes ISP Pipeline: `./build/isp_main network/frame_XXX.raw`
   - Moves output: `data/output.png` → `output_XXX.png`
3. Detects server disconnect and exits gracefully

**Key Implementation:**
- Buffer management for large frames
- System calls to ISP Pipeline executable
- Proper working directory handling for ISP

## Build

### VM Side (Server)
```bash
cd ~/linux-driver-learning/07-network-streaming
make
```

### macOS Side (Client)
```bash
cd ~/Project/ISP_Pipeline/network
make
```

## Usage

### Step 1: Load Driver
```bash
cd ~/linux-driver-learning/05-interrupt-handling
sudo insmod v2_with_waitqueue.ko

# Verify
ls -l /dev/camera
dmesg | tail
```

### Step 2: Start Server (VM)
```bash
cd ~/linux-driver-learning/07-network-streaming
sudo ./frame_streamer
```

Expected output:
```
Opening /dev/camera...
✓ Device opened
Creating socket...
✓ Bound to port 8080
✓ Listening on port 8080...
Waiting for client connection...
```

### Step 3: Start Client (macOS)
```bash
cd ~/Project/ISP_Pipeline/network
./frame_receiver
```

Expected output:
```
Creating socket...
✓ Socket created
Connecting to 192.168.64.2:8080...
✓ Connected!

=== Receiving and processing frames ===

[1] Received 614400 bytes
[1] Saved RAW: frame_001.raw
[1] Processing with ISP...
[1] ✓ Saved PNG: output_001.png

[2] Received 614400 bytes
[2] Saved RAW: frame_002.raw
[2] Processing with ISP...
[2] ✓ Saved PNG: output_002.png

...

[5] Received 614400 bytes
[5] Saved RAW: frame_005.raw
[5] Processing with ISP...
[5] ✓ Saved PNG: output_005.png

Server closed connection

=== Completed ===
✓ Processed 5 frames
```

### Results
After completion, you'll have:
- `frame_001.raw ~ frame_005.raw` (RAW frames from driver)
- `output_001.png ~ output_005.png` (ISP-processed images)

Each frame shows a slightly different gradient pattern, demonstrating continuous frame capture.

## Network Configuration

**Protocol:** TCP (reliable, ordered delivery)
- **Why TCP over UDP?** Camera frames are critical data; packet loss unacceptable
- **Port:** 8080
- **Frame size:** 614,400 bytes (640×480×2)
- **Bandwidth:** ~300 KB/s (5 frames @ 614 KB each over 10 seconds)
- **Latency:** Sub-100ms (local network)

## Testing

### Quick Network Test (Python)
```bash
cd test
python3 tcp_server.py  # Simple echo server for connectivity test
```

### Verify Frames Are Different
```bash
cd ~/Project/ISP_Pipeline/network
md5 frame_*.raw
```
Each hash should be unique, proving frames are genuinely different.

## Technical Deep Dive

### Blocking I/O with poll()
```c
struct pollfd fds[1];
fds[0].fd = device_fd;
fds[0].events = POLLIN;

while (frame_count < MAX_FRAMES) {
    poll(fds, 1, -1);  // ← Process sleeps here
    // Driver's wake_up_interruptible() wakes us
    
    if (fds[0].revents & POLLIN) {
        read(device_fd, buffer, FRAME_SIZE);
        send(client_fd, buffer, FRAME_SIZE, 0);
    }
}
```

**Benefits:**
- CPU usage ~0% while waiting (process is sleeping)
- Wakes instantly when driver signals data ready
- This is how real camera drivers work!

### Data Flow Timeline
```
Time    Driver (Kernel)              Streamer (User)           Receiver (macOS)
─────────────────────────────────────────────────────────────────────────────
0.0s    timer_callback()
        generate_test_pattern()       poll() sleeping          waiting...
        data_ready = true
        wake_up_interruptible() ───→ WAKE UP!
                                     read() /dev/camera
                                     send() via TCP ─────────→ recv()
                                                               save RAW
                                                               call ISP
                                                               save PNG
2.0s    timer_callback() (Frame 2)
        ...
```

### Integration with ISP Pipeline
The ISP Pipeline expects:
- **Format:** RAW12 stored as uint16_t (2 bytes/pixel)
- **Dimensions:** 640×480
- **Bayer Pattern:** RGGB
- **Bit depth:** 12-bit (values 0-4095)

This matches exactly what the driver generates:
```c
// In Module 05 v2_with_waitqueue.c
#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define BYTES_PER_PIXEL 2  // RAW12 as uint16_t
```

## Performance Metrics

| Metric | Value |
|--------|-------|
| Frame size | 614,400 bytes |
| Transfer time | ~50ms (local network) |
| ISP processing | ~2,000 μs (2ms) per frame |
| Total latency | <100ms (capture to PNG) |
| Network overhead | Negligible (<1% CPU) |

## Learning Outcomes

### Network Programming
- ✅ TCP socket API (socket, bind, listen, accept, connect)
- ✅ Client-server architecture design
- ✅ Large data transfer strategies
- ✅ Connection lifecycle management

### System Integration
- ✅ User-space ↔ Kernel-space communication
- ✅ Blocking I/O with poll() (Module 04 + 05)
- ✅ Cross-machine integration (VM ↔ macOS)
- ✅ Process coordination and synchronization

### Real-World Patterns
- ✅ Edge-to-Cloud architecture (Driver → Network → Processing)
- ✅ Producer-consumer pattern
- ✅ Decoupled system design (driver independent of ISP)

## Future Enhancements

- [ ] Add protocol header (magic number, sequence, CRC)
- [ ] Implement flow control and buffering
- [ ] Support multiple simultaneous clients
- [ ] Add frame metadata (timestamp, exposure, gain)
- [ ] Implement graceful error recovery
- [ ] Add performance monitoring and statistics

## Connection to Other Modules

**Module 05 (Interrupt Handling):**
- Provides the frame data via `/dev/camera`
- Timer interrupt simulates camera capture
- Wait queue + wake_up mechanism

**Module 04 (Poll/Select):**
- Blocking I/O pattern used by streamer
- CPU-efficient waiting

**ISP Pipeline (External Project):**
- Consumes RAW frames
- 7-stage processing
- CUDA acceleration (1930x speedup)

## Troubleshooting

### "Failed to open device"
```bash
# Check if driver is loaded
lsmod | grep v2_with_waitqueue

# Check if device exists
ls -l /dev/camera

# Reload driver
cd ~/linux-driver-learning/05-interrupt-handling
sudo rmmod v2_with_waitqueue
sudo insmod v2_with_waitqueue.ko
```

### "Connection refused"
```bash
# Check if server is running
ps aux | grep frame_streamer

# Check if port is in use
sudo netstat -tlnp | grep 8080

# Check firewall (if any)
sudo ufw status
```

### "ISP processing failed"
```bash
# Verify ISP Pipeline is built
ls ~/Project/ISP_Pipeline/build/isp_main

# Test ISP manually
cd ~/Project/ISP_Pipeline
./build/isp_main network/frame_001.raw
```

## Files
```
07-network-streaming/
├── frame_streamer.c       # Server (VM side)
├── Makefile
├── README.md              # This file
└── test/
    └── tcp_server.py      # Simple echo server for testing
```

## References

- [ISP Pipeline Repository](https://github.com/dust2080/ISP_Pipeline)
- [Module 05: Interrupt Handling](../05-interrupt-handling/)
- [Module 04: Poll/Select](../04-poll-select/)
- [Linux Socket Programming Guide](https://man7.org/linux/man-pages/man7/socket.7.html)

---

**Status:** ✅ Completed | **Last Updated:** December 17, 2024
