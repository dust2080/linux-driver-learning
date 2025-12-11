/*
 * ioctl_cmd.h - ioctl command definitions
 * 
 * This header file is shared between kernel driver and user space application.
 * It defines the ioctl commands and data structures for communication.
 */

#ifndef IOCTL_CMD_H
#define IOCTL_CMD_H

#include <linux/ioctl.h>

/* Device magic number - must be unique in the system */
#define IOCTL_MAGIC 'I'

/* 
 * Device parameter structure
 * This structure holds ISP-like parameters that can be controlled
 */
struct device_params {
    unsigned int gain;          /* Image gain (0-100) */
    unsigned int exposure;      /* Exposure time in ms (1-1000) */
    unsigned int wb_temp;       /* White balance temperature in K (2000-10000) */
};

/*
 * Device status structure
 * Used to query current device state
 */
struct device_status {
    unsigned int is_streaming;  /* 1 if device is active, 0 otherwise */
    unsigned int frame_count;   /* Number of frames processed */
    struct device_params params; /* Current parameters */
};

/* 
 * ioctl command definitions using _IO, _IOR, _IOW, _IOWR macros
 * 
 * _IO(magic, number)           - no data transfer
 * _IOR(magic, number, type)    - read data from driver (driver -> user)
 * _IOW(magic, number, type)    - write data to driver (user -> driver)
 * _IOWR(magic, number, type)   - read and write data
 */

/* Reset device to default parameters */
#define IOCTL_RESET         _IO(IOCTL_MAGIC, 0)

/* Set device parameters (user -> kernel) */
#define IOCTL_SET_PARAMS    _IOW(IOCTL_MAGIC, 1, struct device_params)

/* Get device parameters (kernel -> user) */
#define IOCTL_GET_PARAMS    _IOR(IOCTL_MAGIC, 2, struct device_params)

/* Get device status (kernel -> user) */
#define IOCTL_GET_STATUS    _IOR(IOCTL_MAGIC, 3, struct device_status)

/* Start/Stop streaming */
#define IOCTL_START_STREAM  _IO(IOCTL_MAGIC, 4)
#define IOCTL_STOP_STREAM   _IO(IOCTL_MAGIC, 5)

/* Version information */
#define DRIVER_VERSION      "1.0.0"
#define DEVICE_NAME         "ioctl_dev"

#endif /* IOCTL_CMD_H */
