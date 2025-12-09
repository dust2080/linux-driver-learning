# Makefile for character device driver

obj-m += chardev.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	sudo rm -f /dev/hello

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

# Load module and create device file
load:
	sudo insmod chardev.ko
	@echo "Getting major number..."
	$(eval MAJOR := $(shell dmesg | grep "chardev: Major number:" | tail -1 | awk '{print $$NF}'))
	@echo "Creating device file /dev/hello with major number $(MAJOR)..."
	sudo mknod /dev/hello c $(MAJOR) 0
	sudo chmod 666 /dev/hello
	@echo "Device ready! Try:"
	@echo "  echo 'test' > /dev/hello"
	@echo "  cat /dev/hello"

# Unload module and remove device file
unload:
	sudo rm -f /dev/hello
	sudo rmmod chardev
	dmesg | tail -5

# Quick test
test:
	@echo "Writing to device..."
	echo "Hello from userspace" | sudo tee /dev/hello
	@echo ""
	@echo "Reading from device..."
	sudo cat /dev/hello
	@echo ""

.PHONY: all clean install load unload test
