CR_C := arm-linux-gnueabihf-
MODULES := /home/dejana/rpi-kernel/linux
obj-m += driver.o

all:
	make ARCH=arm CROSS_COMPILE=$(CR_C) -C $(MODULES) M=$(shell pwd) modules

clean:
	make ARCH=arm CROSS_COMPILE=$(CR_C) -C $(MODULES) M=$(shell pwd) clean
