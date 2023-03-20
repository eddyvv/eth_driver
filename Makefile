
KERNELDIR := /lib/modules/$(shell uname -r)/build

CURRENT_PATH := $(shell pwd)
obj-m := eth_smart_nic_250soc.o

EXTRA_CFLAGS += -g
CONFIG_DEBUG_INFO=y
build: kernel_modules

kernel_modules:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean


