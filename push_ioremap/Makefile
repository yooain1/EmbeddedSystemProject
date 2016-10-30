obj-m = ioremap_push_dd.o

KDIR = /root/work/kernel-2.6.35
PWD = $(shell pwd)
CC = arm-linux-gcc

all:module clean

module:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
app:
	$(CC) -static -o ioremap_push_test ioremap_push_test.c
clean:
#	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf *.o
	rm -rf *.order
	rm -rf Module.symvers
