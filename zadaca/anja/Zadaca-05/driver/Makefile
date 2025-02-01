CR_C := arm-linux-gnueabihf-
MODULES := /home/anja/rpi-kernel/linux
obj-m += ioctl.o

all:
	make ARCH=arm CROSS_COMPILE=$(CR_C) -C $(MODULES) M=$(shell pwd) modules

clean:
	make ARCH=arm CROSS_COMPILE=$(CR_C) -C $(MODULES) M=$(shell pwd) clean
