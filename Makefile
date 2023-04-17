KERNELDIR := /lib/modules/$(shell uname -r)/build
CURRENT_PATH := $(shell pwd)

BUILD_DIR := build
MODULE_NAME := xtic_nic
ccflags-y += -I$(shell pwd)

obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-objs := xtic_enet_main.o xtic_enet_dma.o xtic_enet_cdev.o


EXTRA_CFLAGS += -g
CONFIG_DEBUG_INFO=y
build: kernel_modules

app:
	g++ -o  main main.c -g

kernel_modules:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules

install:
	sudo rmmod e1000
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	sudo insmod ./$(MODULE_NAME).ko

clean:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	sudo rmmod $(MODULE_NAME)
	sudo insmod /lib/modules/$(shell uname -r)/kernel/drivers/net/ethernet/intel/e1000/e1000.ko


