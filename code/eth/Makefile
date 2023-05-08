KERNELDIR := /lib/modules/$(shell uname -r)/build
CURRENT_PATH := $(shell pwd)
CURRENT_KERNEL_VERSION := $(shell uname -r)
KERNEL_VERSION_5_15 := 5.15.0-60-generic
KERNEL_VERSION_5_4 := 5.4.0-147-generic

ifeq ($(CURRENT_KERNEL_VERSION),$(KERNEL_VERSION_5_15))
EXTRA_CFLAGS += -DLINUX_5_15
endif
ifeq ($(CURRENT_KERNEL_VERSION),$(KERNEL_VERSION_5_4))
EXTRA_CFLAGS += -DLINUX_5_4
endif

MODULE_NAME := xtic_nic

obj-m := $(MODULE_NAME).o

$(MODULE_NAME)-objs := xtic_enet_main.o xtic_enet_dma.o xtic_enet_cdev.o

all:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules

clean:
	make -C $(KERNELDIR) M=$(CURDIR) clean