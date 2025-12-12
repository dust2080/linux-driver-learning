/*
 * poll_test.c - User space test program for poll/select driver
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>

#define DEVICE_PATH "/dev/poll_device"
#define BUFFER_SIZE 1024

#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_RESET   "\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

void print_test_header(const char *test_name)
{
    printf("\n" COLOR_CYAN);
    printf("========================================\n");
    printf("Test: %s\n", test_name);
    printf("========================================\n" COLOR_RESET);
}

void print_success(const char *message)
{
    printf(COLOR_GREEN "✓ %s\n" COLOR_RESET, message);
    tests_passed++;
}

void print_error(const char *message)
{
    printf(COLOR_RED "✗ %s\n" COLOR_RESET, message);
    tests_failed++;
}

void print_info(const char *message)
{
    printf(COLOR_BLUE "ℹ %s\n" COLOR_RESET, message);
}

struct writer_args {
    const char *device_path;
    const char *message;
    int delay_ms;
};

void *writer_thread(void *arg)
{
    struct writer_args *args = (struct writer_args *)arg;
    int fd;
    
    usleep(args->delay_ms * 1000);
    
    fd = open(args->device_path, O_WRONLY);
    if (fd < 0) {
        perror("Writer thread: open failed");
        return NULL;
    }
    
    printf(COLOR_YELLOW "Writer: Writing '%s' after %d ms\n" COLOR_RESET,
           args->message, args->delay_ms);
    
    if (write(fd, args->message, strlen(args->message)) < 0) {
        perror("Writer thread: write failed");
    }
    
    close(fd);
    return NULL;
}

void test_poll_timeout(void)
{
    print_test_header("Basic poll() with timeout (no data)");
    
    int fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open");
        print_error("Failed to open device");
        return;
    }
    
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    
    print_info("Calling poll() with 2 second timeout...");
    
    int ret = poll(fds, 1, 2000);
    
    if (ret == 0) {
        print_success("poll() timed out as expected (no data available)");
    } else if (ret < 0) {
        perror("poll");
        print_error("poll() failed");
    } else {
        print_error("poll() returned unexpectedly");
    }
    
    close(fd);
}

void test_poll_with_data(void)
{
    print_test_header("poll() with data available");
    
    int fd_read = open(DEVICE_PATH, O_RDONLY);
    int fd_write = open(DEVICE_PATH, O_WRONLY);
    
    if (fd_read < 0 || fd_write < 0) {
        perror("open");
        print_error("Failed to open device");
        return;
    }
    
    const char *msg = "Hello from poll test!";
    print_info("Writing data to device...");
    if (write(fd_write, msg, strlen(msg)) < 0) {
        perror("write");
        print_error("Write failed");
        close(fd_read);
        close(fd_write);
        return;
    }
    
    struct pollfd fds[1];
    fds[0].fd = fd_read;
    fds[0].events = POLLIN;
    
    print_info("Calling poll() (should return immediately)...");
    int ret = poll(fds, 1, 5000);
    
    if (ret > 0) {
        if (fds[0].revents & POLLIN) {
            print_success("poll() detected data available (POLLIN)");
            
            char buffer[BUFFER_SIZE];
            ssize_t bytes = read(fd_read, buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                printf(COLOR_GREEN "✓ Read data: '%s'\n" COLOR_RESET, buffer);
            }
        } else {
            print_error("poll() returned but POLLIN not set");
        }
    } else if (ret == 0) {
        print_error("poll() timed out");
    } else {
        perror("poll");
        print_error("poll() failed");
    }
    
    close(fd_read);
    close(fd_write);
}

void test_poll_blocking(void)
{
    print_test_header("poll() blocking until data arrives");
    
    int fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open");
        print_error("Failed to open device");
        return;
    }
    
    pthread_t thread;
    struct writer_args args = {
        .device_path = DEVICE_PATH,
        .message = "Data from writer thread",
        .delay_ms = 1000
    };
    
    if (pthread_create(&thread, NULL, writer_thread, &args) != 0) {
        perror("pthread_create");
        print_error("Failed to create writer thread");
        close(fd);
        return;
    }
    
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    
    print_info("Calling poll() (will block until writer writes data)...");
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    int ret = poll(fds, 1, 5000);
    
    gettimeofday(&end, NULL);
    long elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 +
                      (end.tv_usec - start.tv_usec) / 1000;
    
    if (ret > 0 && (fds[0].revents & POLLIN)) {
        printf(COLOR_GREEN "✓ poll() unblocked after %ld ms\n" COLOR_RESET,
               elapsed_ms);
        
        char buffer[BUFFER_SIZE];
        ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            printf(COLOR_GREEN "✓ Read data: '%s'\n" COLOR_RESET, buffer);
        }
        
        if (elapsed_ms >= 900 && elapsed_ms <= 1500) {
            print_success("Timing correct (~1 second)");
        } else {
            print_error("Timing incorrect");
        }
    } else {
        print_error("poll() failed or timed out");
    }
    
    pthread_join(thread, NULL);
    close(fd);
}
void test_select(void)
{
    print_test_header("select() system call");

    int fd_read = open(DEVICE_PATH, O_RDONLY);
    int fd_write = open(DEVICE_PATH, O_WRONLY);

    if (fd_read < 0 || fd_write < 0) {
        perror("open");
        print_error("Failed to open device");
        return;
    }

    const char *msg = "Data for select test";
    if (write(fd_write, msg, strlen(msg)) < 0) {
        perror("write");
        print_error("Write failed");
        close(fd_read);
        close(fd_write);
        return;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd_read, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    print_info("Calling select() (should return immediately)...");
    int ret = select(fd_read + 1, &readfds, NULL, NULL, &timeout);

    if (ret > 0) {
        if (FD_ISSET(fd_read, &readfds)) {
            print_success("select() detected data available");

            char buffer[BUFFER_SIZE];
            ssize_t bytes = read(fd_read, buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                printf(COLOR_GREEN "✓ Read data: '%s'\n" COLOR_RESET, buffer);
            }
        }
    } else if (ret == 0) {
        print_error("select() timed out");
    } else {
        perror("select");
        print_error("select() failed");
    }

    close(fd_read);
    close(fd_write);
}

void test_nonblocking_read(void)
{
    print_test_header("Non-blocking read (O_NONBLOCK)");

    int fd = open(DEVICE_PATH, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        print_error("Failed to open device");
        return;
    }

    char buffer[BUFFER_SIZE];

    print_info("Attempting non-blocking read (no data available)...");
    ssize_t bytes = read(fd, buffer, sizeof(buffer));

    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            print_success("Non-blocking read returned EAGAIN as expected");
        } else {
            perror("read");
            print_error("Unexpected error");
        }
    } else {
        print_error("Non-blocking read should have returned EAGAIN");
    }

    close(fd);
}

void test_multiple_fds(void)
{
    print_test_header("Monitoring multiple file descriptors");

    int fd1 = open(DEVICE_PATH, O_RDONLY);
    int fd2 = open(DEVICE_PATH, O_RDONLY);

    if (fd1 < 0 || fd2 < 0) {
        perror("open");
        print_error("Failed to open devices");
        return;
    }

    pthread_t thread;
    struct writer_args args = {
        .device_path = DEVICE_PATH,
        .message = "Multi-FD test data",
        .delay_ms = 500
    };

    pthread_create(&thread, NULL, writer_thread, &args);

    struct pollfd fds[2];
    fds[0].fd = fd1;
    fds[0].events = POLLIN;
    fds[1].fd = fd2;
    fds[1].events = POLLIN;

    print_info("Monitoring 2 file descriptors with poll()...");
    int ret = poll(fds, 2, 3000);

    if (ret > 0) {
        printf(COLOR_GREEN "✓ poll() detected events on %d FD(s)\n" COLOR_RESET,
               ret);

        if (fds[0].revents & POLLIN) {
            print_success("FD1 ready for reading");
            char buffer[BUFFER_SIZE];
            ssize_t bytes = read(fd1, buffer, sizeof(buffer));
            (void)bytes;
        }
        if (fds[1].revents & POLLIN) {
            print_success("FD2 ready for reading");
        }
    } else {
        print_error("poll() failed or timed out");
    }

    pthread_join(thread, NULL);
    close(fd1);
    close(fd2);
}

int main(int argc, char *argv[])
{
    printf(COLOR_MAGENTA);
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║   Poll/Select Driver - Test Suite                 ║\n");
    printf("║   Testing asynchronous I/O and event notification ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);

    if (access(DEVICE_PATH, F_OK) != 0) {
        printf(COLOR_RED "Error: Device %s not found\n" COLOR_RESET,
               DEVICE_PATH);
        printf("Please load the driver first: sudo insmod poll_driver.ko\n");
        return 1;
    }

    int test_num = (argc > 1) ? atoi(argv[1]) : 0;

    if (test_num == 0 || test_num == 1)
        test_poll_timeout();

    if (test_num == 0 || test_num == 2)
        test_poll_with_data();

    if (test_num == 0 || test_num == 3)
        test_poll_blocking();

    if (test_num == 0 || test_num == 4)
        test_select();

    if (test_num == 0 || test_num == 5)
        test_nonblocking_read();

    if (test_num == 0 || test_num == 6)
        test_multiple_fds();

    printf("\n" COLOR_MAGENTA);
    printf("========================================\n");
    printf("Test Summary\n");
    printf("========================================\n" COLOR_RESET);
    printf(COLOR_GREEN "Passed: %d\n" COLOR_RESET, tests_passed);
    if (tests_failed > 0) {
        printf(COLOR_RED "Failed: %d\n" COLOR_RESET, tests_failed);
    }
    printf("Total: %d\n", tests_passed + tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
