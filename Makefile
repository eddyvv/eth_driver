
KERNELDIR := /lib/modules/$(shell uname -r)/build
#KERNELDIR := /usr/src/linux-headers-5.15.0-56-generic

CURRENT_PATH := $(shell pwd)
obj-m := eth_smart_nic_250soc.o

build: kernel_modules

kernel_modules:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean


