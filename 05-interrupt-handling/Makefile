# Makefile for Module 05: Interrupt Handling

# Kernel modules to build
obj-m += v1_timer_interrupt.o
obj-m += v2_with_waitqueue.o

# Kernel build directory
KDIR := /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

# Default target: build all
all: modules userspace

# Build kernel modules
modules:
	make -C $(KDIR) M=$(PWD) modules

# Build user space test program
userspace:
	gcc -Wall -o interrupt_test interrupt_test.c

# Clean build artifacts
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f interrupt_test

# Install module (for testing)
install:
	sudo insmod v2_with_waitqueue.ko

# Remove module
uninstall:
	sudo rmmod v2_with_waitqueue

# Show kernel log
log:
	dmesg | tail -50

# Help
help:
	@echo "Available targets:"
	@echo "  make          - Build kernel modules and test program"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make install  - Load v2 module"
	@echo "  make uninstall- Unload v2 module"
	@echo "  make log      - Show recent kernel messages"
