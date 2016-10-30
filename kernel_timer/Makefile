CC = arm-linux-gcc
KDIR = /root/work/kernel-2.6.35

obj-m = clock_timer_dd.o

PWD = $(shell pwd)

TEST_TARGET = test_timer_led
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
