obj-m = ioremap_fnd_dd.o

CC = arm-linux-gcc

KDIR = /root/work/kernel-2.6.35/

PWD = $(shell pwd)

TEST_TARGET = ioremap_fnd_test
TEST_SRCS = $(TEST_TARGET).c

all: module clean

module:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
test_pgm:
	$(CC) $(TEST_SRCS) -o $(TEST_TARGET)
clean:
#	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf *.o
	rm -rf *.order
	rm -rf Module.symvers
