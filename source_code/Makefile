# obj-m is used for modules that will be compiled as loadable kernel modules.
# `+=` indicates the values are appended.
obj-m += producer_consumer.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
