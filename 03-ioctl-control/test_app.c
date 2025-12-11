/*
 * test_app.c - User space application to test ioctl driver
 * 
 * This program demonstrates how to use ioctl() to control a device driver.
 * It tests all the ioctl commands defined in ioctl_cmd.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "ioctl_cmd.h"

#define DEVICE_PATH "/dev/ioctl_dev"

/*
 * Print device parameters in a readable format
 */
static void print_params(const struct device_params *params)
{
    printf("  Gain:         %u (0-100)\n", params->gain);
    printf("  Exposure:     %u ms (1-1000)\n", params->exposure);
    printf("  White Balance: %u K (2000-10000)\n", params->wb_temp);
}

/*
 * Print device status in a readable format
 */
static void print_status(const struct device_status *status)
{
    printf("Device Status:\n");
    printf("  Streaming:    %s\n", status->is_streaming ? "Yes" : "No");
    printf("  Frame Count:  %u\n", status->frame_count);
    printf("Current Parameters:\n");
    print_params(&status->params);
}

/*
 * Test 1: Reset device to defaults
 */
static int test_reset(int fd)
{
    printf("\n========================================\n");
    printf("Test 1: Reset Device\n");
    printf("========================================\n");
    
    if (ioctl(fd, IOCTL_RESET, NULL) < 0) {
        perror("IOCTL_RESET failed");
        return -1;
    }
    
    printf("✓ Device reset successfully\n");
    return 0;
}

/*
 * Test 2: Get current parameters
 */
static int test_get_params(int fd)
{
    struct device_params params;
    
    printf("\n========================================\n");
    printf("Test 2: Get Current Parameters\n");
    printf("========================================\n");
    
    if (ioctl(fd, IOCTL_GET_PARAMS, &params) < 0) {
        perror("IOCTL_GET_PARAMS failed");
        return -1;
    }
    
    printf("Current Parameters:\n");
    print_params(&params);
    printf("✓ Get parameters successfully\n");
    return 0;
}

/*
 * Test 3: Set new parameters
 */
static int test_set_params(int fd)
{
    struct device_params params;
    
    printf("\n========================================\n");
    printf("Test 3: Set New Parameters\n");
    printf("========================================\n");
    
    /* Set custom parameters */
    params.gain = 75;
    params.exposure = 200;
    params.wb_temp = 6500;
    
    printf("Setting new parameters:\n");
    print_params(&params);
    
    if (ioctl(fd, IOCTL_SET_PARAMS, &params) < 0) {
        perror("IOCTL_SET_PARAMS failed");
        return -1;
    }
    
    printf("✓ Set parameters successfully\n");
    
    /* Verify by reading back */
    printf("\nVerifying parameters...\n");
    if (ioctl(fd, IOCTL_GET_PARAMS, &params) < 0) {
        perror("IOCTL_GET_PARAMS failed");
        return -1;
    }
    
    printf("Parameters after setting:\n");
    print_params(&params);
    return 0;
}

/*
 * Test 4: Test parameter validation
 */
static int test_invalid_params(int fd)
{
    struct device_params params;
    
    printf("\n========================================\n");
    printf("Test 4: Test Invalid Parameters\n");
    printf("========================================\n");
    
    /* Test invalid gain (> 100) */
    printf("Testing invalid gain (150)...\n");
    params.gain = 150;
    params.exposure = 100;
    params.wb_temp = 5500;
    
    if (ioctl(fd, IOCTL_SET_PARAMS, &params) < 0) {
        printf("✓ Invalid gain rejected (expected): %s\n", strerror(errno));
    } else {
        printf("✗ Invalid gain accepted (unexpected!)\n");
        return -1;
    }
    
    /* Test invalid exposure (> 1000) */
    printf("\nTesting invalid exposure (2000)...\n");
    params.gain = 50;
    params.exposure = 2000;
    params.wb_temp = 5500;
    
    if (ioctl(fd, IOCTL_SET_PARAMS, &params) < 0) {
        printf("✓ Invalid exposure rejected (expected): %s\n", strerror(errno));
    } else {
        printf("✗ Invalid exposure accepted (unexpected!)\n");
        return -1;
    }
    
    /* Test invalid white balance (< 2000) */
    printf("\nTesting invalid white balance (1000)...\n");
    params.gain = 50;
    params.exposure = 100;
    params.wb_temp = 1000;
    
    if (ioctl(fd, IOCTL_SET_PARAMS, &params) < 0) {
        printf("✓ Invalid white balance rejected (expected): %s\n", strerror(errno));
    } else {
        printf("✗ Invalid white balance accepted (unexpected!)\n");
        return -1;
    }
    
    printf("\n✓ Parameter validation working correctly\n");
    return 0;
}

/*
 * Test 5: Get device status
 */
static int test_get_status(int fd)
{
    struct device_status status;
    
    printf("\n========================================\n");
    printf("Test 5: Get Device Status\n");
    printf("========================================\n");
    
    if (ioctl(fd, IOCTL_GET_STATUS, &status) < 0) {
        perror("IOCTL_GET_STATUS failed");
        return -1;
    }
    
    print_status(&status);
    printf("✓ Get status successfully\n");
    return 0;
}

/*
 * Test 6: Start streaming
 */
static int test_start_stream(int fd)
{
    printf("\n========================================\n");
    printf("Test 6: Start Streaming\n");
    printf("========================================\n");
    
    if (ioctl(fd, IOCTL_START_STREAM, NULL) < 0) {
        perror("IOCTL_START_STREAM failed");
        return -1;
    }
    
    printf("✓ Streaming started successfully\n");
    
    /* Verify streaming state */
    struct device_status status;
    if (ioctl(fd, IOCTL_GET_STATUS, &status) < 0) {
        perror("IOCTL_GET_STATUS failed");
        return -1;
    }
    
    printf("Streaming state: %s\n", status.is_streaming ? "Active" : "Inactive");
    return 0;
}

/*
 * Test 7: Try to start streaming again (should fail with EBUSY)
 */
static int test_start_stream_again(int fd)
{
    printf("\n========================================\n");
    printf("Test 7: Start Streaming Again (Should Fail)\n");
    printf("========================================\n");
    
    if (ioctl(fd, IOCTL_START_STREAM, NULL) < 0) {
        if (errno == EBUSY) {
            printf("✓ Second start rejected with EBUSY (expected): %s\n", strerror(errno));
            return 0;
        } else {
            printf("✗ Failed with unexpected error: %s\n", strerror(errno));
            return -1;
        }
    }
    
    printf("✗ Second start accepted (unexpected!)\n");
    return -1;
}

/*
 * Test 8: Stop streaming
 */
static int test_stop_stream(int fd)
{
    printf("\n========================================\n");
    printf("Test 8: Stop Streaming\n");
    printf("========================================\n");
    
    if (ioctl(fd, IOCTL_STOP_STREAM, NULL) < 0) {
        perror("IOCTL_STOP_STREAM failed");
        return -1;
    }
    
    printf("✓ Streaming stopped successfully\n");
    
    /* Verify streaming state */
    struct device_status status;
    if (ioctl(fd, IOCTL_GET_STATUS, &status) < 0) {
        perror("IOCTL_GET_STATUS failed");
        return -1;
    }
    
    printf("Streaming state: %s\n", status.is_streaming ? "Active" : "Inactive");
    printf("Total frames processed: %u\n", status.frame_count);
    return 0;
}

/*
 * Main function - runs all tests
 */
int main(int argc, char *argv[])
{
    int fd;
    int test_count = 0;
    int test_passed = 0;
    
    printf("========================================\n");
    printf("ioctl Driver Test Application\n");
    printf("========================================\n");
    
    /* Open device */
    printf("\nOpening device %s...\n", DEVICE_PATH);
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        printf("\nMake sure the driver is loaded:\n");
        printf("  sudo insmod ioctl_driver.ko\n");
        printf("  ls -l /dev/ioctl_dev\n");
        return EXIT_FAILURE;
    }
    printf("✓ Device opened successfully (fd=%d)\n", fd);
    
    /* Run all tests */
    #define RUN_TEST(test_func) do { \
        test_count++; \
        if (test_func(fd) == 0) { \
            test_passed++; \
        } \
    } while(0)
    
    RUN_TEST(test_reset);
    RUN_TEST(test_get_params);
    RUN_TEST(test_set_params);
    RUN_TEST(test_invalid_params);
    RUN_TEST(test_get_status);
    RUN_TEST(test_start_stream);
    RUN_TEST(test_start_stream_again);
    RUN_TEST(test_stop_stream);
    
    /* Summary */
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Total tests:  %d\n", test_count);
    printf("Passed:       %d\n", test_passed);
    printf("Failed:       %d\n", test_count - test_passed);
    
    if (test_passed == test_count) {
        printf("\n✓ All tests passed!\n");
    } else {
        printf("\n✗ Some tests failed!\n");
    }
    
    /* Close device */
    close(fd);
    printf("\nDevice closed.\n");
    
    return (test_passed == test_count) ? EXIT_SUCCESS : EXIT_FAILURE;
}
