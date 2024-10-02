CR_C := arm-linux-gnueabihf-
MODULES := ~/linux
obj-m += driver.o

all:
	make ARCH=arm CROSS_COMPILE=$(CR_C) -C $(MODULES) M=$(shell pwd) modules

clean:
	make ARCH=arm CROSS_COMPILE=$(CR_C) -C $(MODULES) M=$(shell pwd) clean
