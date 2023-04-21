KERNELDIR := /lib/modules/$(shell uname -r)/build
CURRENT_PATH := $(shell pwd)

CURRENT_KERNEL_VERSION := $(shell uname -r)
KERNEL_VERSION_5_15 := 5.15.0-60-generic

BUILD_DIR := build
MODULE_NAME := xtic_nic
ccflags-y += -I$(shell pwd)

obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-objs := xtic_enet_main.o xtic_enet_dma.o xtic_enet_cdev.o

EXTRA_CFLAGS += -g
CONFIG_DEBUG_INFO=y
build: kernel_modules

app:
	gcc -o mainApp main.c -g

kernel_modules:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	make app

install:
ifeq ($(CURRENT_KERNEL_VERSION),$(KERNEL_VERSION_5_15))
	sudo rmmod e1000
endif
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	sudo insmod ./$(MODULE_NAME).ko
	make app

clean:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm -rf main mainApp
	sudo rmmod $(MODULE_NAME)
ifeq ($(CURRENT_KERNEL_VERSION),$(KERNEL_VERSION_5_15))
	sudo insmod /lib/modules/$(shell uname -r)/kernel/drivers/net/ethernet/intel/e1000/e1000.ko
endif


