MK_INC := $(PWD)/mk
include $(MK_INC)/common.mk

KBUILD_EXTRA_SYMBOLS = $(PWD)/eth/Module.symvers
MODULE_NAME := xib

obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-objs := main.o verbs.o rnic.o mem.o

all:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules

clean:
	make -C $(KERNELDIR) M=$(CURDIR) clean