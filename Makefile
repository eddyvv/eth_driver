KERNELDIR := /lib/modules/$(shell uname -r)/build
CURRENT_PATH := $(shell pwd)

BUILD_DIR := build
MODULE_NAME := xtic_enet_main
ccflags-y += -I$(shell pwd)

obj-m := $(MODULE_NAME).o

# $(MODULE_NAME)-objs := $(MODULE_NAME).o

EXTRA_CFLAGS += -g
CONFIG_DEBUG_INFO=y
build: kernel_modules

kernel_modules:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	sudo cp $(MODULE_NAME).ko /lib/modules/$(shell uname -r)/kernel/drivers

install:
	sudo rmmod e1000
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	sudo cp ./$(MODULE_NAME).ko /lib/modules/$(shell uname -r)/kernel/drivers
	sudo insmod ./$(MODULE_NAME).ko
clean:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	sudo rm /lib/modules/$(shell uname -r)/kernel/drivers/$(MODULE_NAME).ko
	rm -rf $(BUILD_DIR)
	sudo rmmod $(MODULE_NAME)
	sudo insmod /lib/modules/5.15.0-60-generic/kernel/drivers/net/ethernet/intel/e1000/e1000.ko


