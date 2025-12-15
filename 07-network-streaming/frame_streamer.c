// frame_streamer.c - Read from driver using poll() and send via network
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEVICE_PATH "/dev/camera"
#define PORT 8080
#define FRAME_SIZE 614400
#define MAX_FRAMES 5  // Limit to 5 frames for demo

int main() {
    int device_fd, server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char *buffer;
    ssize_t bytes_read;
    struct pollfd fds[1];
    
    // Allocate buffer for frame data
    buffer = malloc(FRAME_SIZE);
    if (!buffer) {
        perror("Failed to allocate buffer");
        return 1;
    }
    
    // 1. Open camera device
    printf("Opening %s...\n", DEVICE_PATH);
    device_fd = open(DEVICE_PATH, O_RDONLY);
    if (device_fd < 0) {
        perror("Failed to open device");
        free(buffer);
        return 1;
    }
    printf("✓ Device opened\n");
    
    // 2. Create TCP socket
    printf("Creating socket...\n");
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        close(device_fd);
        free(buffer);
        return 1;
    }
    
    // Allow port reuse
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 3. Bind to port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        close(device_fd);
        free(buffer);
        return 1;
    }
    printf("✓ Bound to port %d\n", PORT);
    
    // 4. Start listening
    if (listen(server_fd, 1) < 0) {
        perror("Listen failed");
        close(server_fd);
        close(device_fd);
        free(buffer);
        return 1;
    }
    printf("✓ Listening on port %d...\n", PORT);
    
    // 5. Accept connection
    printf("Waiting for client connection...\n");
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Accept failed");
        close(server_fd);
        close(device_fd);
        free(buffer);
        return 1;
    }
    printf("✓ Client connected from %s:%d\n", 
           inet_ntoa(client_addr.sin_addr), 
           ntohs(client_addr.sin_port));
    
    // 6. Setup poll structure
    fds[0].fd = device_fd;
    fds[0].events = POLLIN;
    
    // 7. Main loop: poll, read, and send
    printf("\n=== Starting frame streaming (640x480 RAW) ===\n");
    printf("Will transmit %d frames and stop.\n", MAX_FRAMES);
    int frame_count = 0;
    
    while (frame_count < MAX_FRAMES) {
        // Wait for data ready (blocks until interrupt wakes us up)
        int ret = poll(fds, 1, -1);
        
        if (ret < 0) {
            perror("Poll failed");
            break;
        }
        
        // Check if data is ready
        if (fds[0].revents & POLLIN) {
            // Data ready, now read
            bytes_read = read(device_fd, buffer, FRAME_SIZE);
            
            if (bytes_read < 0) {
                perror("Read from device failed");
                break;
            }
            
            if (bytes_read == 0) {
                printf("No more data from device\n");
                break;
            }
            
            frame_count++;
            printf("[%d] Read %zd bytes (640x480 RAW frame)\n", 
                   frame_count, bytes_read);
            
            // Send via network
            ssize_t total_sent = 0;
            while (total_sent < bytes_read) {
                ssize_t sent = send(client_fd, buffer + total_sent, 
                                   bytes_read - total_sent, 0);
                if (sent < 0) {
                    perror("Send failed");
                    goto cleanup;
                }
                total_sent += sent;
            }
            printf("[%d] Sent %zd bytes\n", frame_count, total_sent);
        }
    }
    
    printf("\n✓ Transmitted %d frames. Closing connection.\n", frame_count);
    
cleanup:
    // 8. Cleanup
    printf("=== Cleaning up ===\n");
    close(client_fd);
    close(server_fd);
    close(device_fd);
    free(buffer);
    
    return 0;
}
