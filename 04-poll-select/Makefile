# Makefile for Poll/Select Driver Module
#
# Targets:
#   all        - Build both kernel module and test application (default)
#   module     - Build only kernel module
#   app        - Build only test application
#   clean      - Remove build artifacts
#   install    - Load the kernel module
#   uninstall  - Unload the kernel module
#   test       - Run test application
#   fulltest   - Build, install, test, and uninstall
#   help       - Show available targets

# Kernel module name
MODULE_NAME := poll_driver
obj-m += $(MODULE_NAME).o

# Test application
TEST_APP := poll_test

# Kernel build directory
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

# Compiler flags for test application
CFLAGS := -Wall -Wextra -O2 -pthread

# Default target: build everything
all: module app

# Build kernel module
module:
	@echo "Building kernel module..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	@echo "Kernel module built successfully"

# Build test application
app: $(TEST_APP).c
	@echo "Building test application..."
	gcc $(CFLAGS) -o $(TEST_APP) $(TEST_APP).c
	@echo "Test application built successfully"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f $(TEST_APP)
	rm -f *.o *.ko *.mod.* Module.symvers modules.order
	@echo "Clean complete"

# Install (load) the kernel module
install: module
	@echo "Loading kernel module..."
	sudo insmod $(MODULE_NAME).ko
	@echo "Kernel module loaded"
	@echo "Device created at /dev/poll_device"
	@sudo dmesg | tail -10

# Uninstall (unload) the kernel module
uninstall:
	@echo "Unloading kernel module..."
	sudo rmmod $(MODULE_NAME)
	@echo "Kernel module unloaded"
	@sudo dmesg | tail -5

# Run test application
test: app
	@echo "Running test application..."
	@echo ""
	./$(TEST_APP)
	@echo ""
	@echo "Kernel logs:"
	@sudo dmesg | tail -20

# Full test workflow: build, install, test, uninstall
fulltest: all
	@echo "===== Full Test Workflow ====="
	@echo ""
	@echo "Step 1: Loading kernel module..."
	@sudo insmod $(MODULE_NAME).ko || true
	@sleep 1
	@echo ""
	@echo "Step 2: Setting permissions..."
	@sudo chmod 666 /dev/poll_device || true
	@echo ""
	@echo "Step 3: Running tests..."
	@./$(TEST_APP) || true
	@echo ""
	@echo "Step 4: Checking kernel logs..."
	@sudo dmesg | tail -30
	@echo ""
	@echo "Step 5: Unloading kernel module..."
	@sudo rmmod $(MODULE_NAME) || true
	@echo ""
	@echo "===== Full Test Complete ====="

# Show kernel module info
info:
	@echo "Module information:"
	@modinfo $(MODULE_NAME).ko

# Show device info
device-info:
	@echo "Device information:"
	@ls -l /dev/poll_device
	@echo ""
	@echo "Module loaded:"
	@lsmod | grep poll_driver

# Run a specific test
test1: app
	./$(TEST_APP) 1

test2: app
	./$(TEST_APP) 2

test3: app
	./$(TEST_APP) 3

test4: app
	./$(TEST_APP) 4

test5: app
	./$(TEST_APP) 5

test6: app
	./$(TEST_APP) 6

# Help target
help:
	@echo "Available targets:"
	@echo "  make all        - Build kernel module and test application (default)"
	@echo "  make module     - Build only kernel module"
	@echo "  make app        - Build only test application"
	@echo "  make clean      - Remove build artifacts"
	@echo "  make install    - Load kernel module"
	@echo "  make uninstall  - Unload kernel module"
	@echo "  make test       - Run all tests"
	@echo "  make test1-6    - Run specific test (1-6)"
	@echo "  make fulltest   - Complete workflow: build, load, test, unload"
	@echo "  make info       - Show module information"
	@echo "  make device-info - Show device information"
	@echo "  make help       - Show this help message"
	@echo ""
	@echo "Quick start:"
	@echo "  1. make all      # Build everything"
	@echo "  2. make fulltest # Run complete test"
	@echo ""
	@echo "Manual workflow:"
	@echo "  1. make all      # Build"
	@echo "  2. make install  # Load driver"
	@echo "  3. make test     # Run tests"
	@echo "  4. make uninstall # Unload driver"

.PHONY: all module app clean install uninstall test fulltest info device-info help test1 test2 test3 test4 test5 test6
