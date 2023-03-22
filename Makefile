KERNELDIR := /lib/modules/$(shell uname -r)/build
CURRENT_PATH := $(shell pwd)

ccflags-y += -I$(shell pwd)

obj-m := eth_smart_nic_250soc.o

EXTRA_CFLAGS += -g
CONFIG_DEBUG_INFO=y
build: kernel_modules

kernel_modules:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	sudo cp ./eth_smart_nic_250soc.ko /lib/modules/$(shell uname -r)/kernel/drivers

install:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	sudo cp ./eth_smart_nic_250soc.ko /lib/modules/$(shell uname -r)/kernel/drivers
	sudo insmod eth_smart_nic_250soc.ko
clean:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	sudo rm /lib/modules/$(shell uname -r)/kernel/drivers/eth_smart_nic_250soc.ko
	sudo rmmod eth_smart_nic_250soc


