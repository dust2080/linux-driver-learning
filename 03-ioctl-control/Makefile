# Makefile for ioctl driver and test application

# Kernel module name
MODULE_NAME := ioctl_driver
obj-m += $(MODULE_NAME).o

# User space application
APP_NAME := test_app

# Kernel build directory
KDIR := /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

# Compiler flags for user space application
CFLAGS := -Wall -Wextra -O2

# Default target: build everything
all: module app

# Build kernel module
module:
	@echo "Building kernel module..."
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Build user space application
app: $(APP_NAME).c ioctl_cmd.h
	@echo "Building user space application..."
	$(CC) $(CFLAGS) -o $(APP_NAME) $(APP_NAME).c

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(APP_NAME)
	rm -f *.o *.ko *.mod.c *.symvers *.order .*.cmd
	rm -rf .tmp_versions

# Install kernel module (requires root)
install: module
	@echo "Installing kernel module..."
	sudo insmod $(MODULE_NAME).ko
	@echo "Module installed. Check with: lsmod | grep $(MODULE_NAME)"
	@echo "Device file: /dev/ioctl_dev"

# Uninstall kernel module (requires root)
uninstall:
	@echo "Uninstalling kernel module..."
	sudo rmmod $(MODULE_NAME)
	@echo "Module uninstalled."

# Run tests (requires module to be installed)
test: app
	@echo "Running tests..."
	./$(APP_NAME)

# Show kernel log messages
log:
	@echo "Showing kernel log (last 30 lines)..."
	sudo dmesg | tail -30

# Full test cycle: install, test, show log, uninstall
fulltest: clean all install test log uninstall

# Help message
help:
	@echo "Available targets:"
	@echo "  all        - Build kernel module and user space application (default)"
	@echo "  module     - Build kernel module only"
	@echo "  app        - Build user space application only"
	@echo "  clean      - Remove all build artifacts"
	@echo "  install    - Install kernel module (requires sudo)"
	@echo "  uninstall  - Uninstall kernel module (requires sudo)"
	@echo "  test       - Run test application (requires module installed)"
	@echo "  log        - Show kernel log messages"
	@echo "  fulltest   - Complete test cycle (clean, build, install, test, uninstall)"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Typical usage:"
	@echo "  make all          # Build everything"
	@echo "  make install      # Load driver"
	@echo "  make test         # Run tests"
	@echo "  make log          # Check kernel messages"
	@echo "  make uninstall    # Unload driver"
	@echo ""
	@echo "Quick test:"
	@echo "  make fulltest     # Do everything in one command"

.PHONY: all module app clean install uninstall test log fulltest help
