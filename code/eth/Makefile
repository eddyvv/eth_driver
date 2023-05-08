MK_INC := $(PWD)/mk
include $(MK_INC)/common.mk

MODULE_NAME := xtic_nic

obj-m := $(MODULE_NAME).o

$(MODULE_NAME)-objs := xtic_enet_main.o xtic_enet_dma.o xtic_enet_cdev.o

all:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules

clean:
	make -C $(KERNELDIR) M=$(CURDIR) clean