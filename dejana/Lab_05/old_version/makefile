obj-m += ioctl.o

KDIR = /lib/modules/$(shell uname -r)

all:
	make -C $(KDIR) M=$(shell pwd) modules

clean:
	make -C $(KDIR) M=$(shell pwd) clean
