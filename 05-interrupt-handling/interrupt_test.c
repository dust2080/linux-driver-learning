/*
 * interrupt_test.c
 * 
 * User space test program for Module 05 v2
 * 
 * This program demonstrates:
 * 1. Opening the device
 * 2. Calling poll() to wait for data
 * 3. Reading data when poll() indicates it's ready
 * 
 * Flow:
 * - poll() blocks (process sleeps)
 * - Kernel timer fires -> interrupt handler -> wake_up()
 * - poll() wakes up and returns
 * - read() gets the frame data
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#define DEVICE_PATH "/dev/interrupt_dev"
#define BUFFER_SIZE 128

int main(int argc, char *argv[])
{
    int fd;
    struct pollfd pfd;
    char buffer[BUFFER_SIZE];
    int count = 0;
    int max_frames = 5;  /* Capture 5 frames by default */
    int ret;
    
    /* Allow user to specify number of frames */
    if (argc > 1) {
        max_frames = atoi(argv[1]);
        if (max_frames <= 0) {
            fprintf(stderr, "Invalid frame count, using default (5)\n");
            max_frames = 5;
        }
    }
    
    printf("========================================\n");
    printf("Interrupt Test Program\n");
    printf("========================================\n");
    printf("Will capture %d frames\n", max_frames);
    printf("Press Ctrl+C to stop early\n\n");
    
    /* Open the device */
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        printf("\nTroubleshooting:\n");
        printf("1. Check if module is loaded: lsmod | grep v2_with_waitqueue\n");
        printf("2. Check if device exists: ls -l /dev/interrupt_dev\n");
        printf("3. Load module: sudo insmod v2_with_waitqueue.ko\n");
        return -1;
    }
    
    printf("Device opened successfully\n");
    printf("Starting to wait for interrupts...\n\n");
    
    /* Setup poll structure */
    pfd.fd = fd;
    pfd.events = POLLIN;  /* We want to know when data is readable */
    
    /* Main loop: wait for frames */
    while (count < max_frames) {
        printf("[Frame %d/%d] Calling poll()... ", count + 1, max_frames);
        fflush(stdout);
        
        /*
         * Call poll() with infinite timeout (-1)
         * This will BLOCK (sleep) until:
         * 1. Data becomes available (interrupt handler calls wake_up)
         * 2. Error occurs
         * 3. Signal received (Ctrl+C)
         */
        ret = poll(&pfd, 1, -1);
        
        if (ret < 0) {
            printf("ERROR\n");
            perror("poll failed");
            break;
        }
        
        if (ret == 0) {
            /* This shouldn't happen with infinite timeout */
            printf("TIMEOUT\n");
            continue;
        }
        
        /* Check what events occurred */
        if (pfd.revents & POLLIN) {
            printf("READY!\n");
            
            /* Data is ready, read it */
            memset(buffer, 0, BUFFER_SIZE);
            ret = read(fd, buffer, BUFFER_SIZE - 1);
            
            if (ret > 0) {
                buffer[ret] = '\0';  /* Ensure null termination */
                printf("                   Read %d bytes: %s", ret, buffer);
                count++;
            } else if (ret == 0) {
                printf("                   Read: EOF\n");
            } else {
                printf("                   Read failed: %s\n", strerror(errno));
            }
        }
        
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            printf("ERROR - Device error\n");
            break;
        }
        
        printf("\n");
    }
    
    printf("========================================\n");
    printf("Test completed!\n");
    printf("Total frames captured: %d\n", count);
    printf("========================================\n");
    
    close(fd);
    return 0;
}
