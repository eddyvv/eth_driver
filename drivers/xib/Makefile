MK_INC := $(PWD)/mk
include $(MK_INC)/common.mk

KERNELDIR_ := /lib/modules/$(shell uname -r)/build

MODULE_NAME := xib

obj-m += $(MODULE_NAME).o

$(MODULE_NAME)-objs := main.o ../eth/xt_roce.o

all:
	$(MAKE) -C $(KERNELDIR_) M=$(CURDIR) modules

clean:
	make -C $(KERNELDIR_) M=$(CURDIR) clean