CR_C := arm-linux-gnueabihf-
MODULES := ~/Fakultet/Programiranje-u-realnom-vremenu/Laboratorije/Lab-4/rpi-kernel/linux/
obj-m += proces.o

all:
	make ARCH=arm CROSS_COMPILE=$(CR_C) -C $(MODULES) M=$(shell pwd) modules

clean:
	make ARCH=arm CROSS_COMPILE=$(CR_C) -C $(MODULES) M=$(shell pwd) clean
