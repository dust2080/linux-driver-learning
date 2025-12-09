# Makefile for hello kernel module

# Object file to build
obj-m += hello.o

# Kernel build directory (adjust if needed)
KDIR := /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

# Default target: build the module
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Clean build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Install module (requires root)
install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

# Load module (requires root)
load:
	sudo insmod hello.ko
	dmesg | tail -5

# Unload module (requires root)
unload:
	sudo rmmod hello
	dmesg | tail -5

.PHONY: all clean install load unload
