KERNELDIR := /lib/modules/$(shell uname -r)/build
CURRENT_KERNEL_VERSION := $(shell uname -r)
KERNEL_VERSION_5_15 := 5.15.0-60-generic
KERNEL_VERSION_5_4 := 5.4.0-147-generic

ifeq ($(CURRENT_KERNEL_VERSION),$(KERNEL_VERSION_5_15))
EXTRA_CFLAGS += -DLINUX_5_15
endif

ifeq ($(CURRENT_KERNEL_VERSION),$(KERNEL_VERSION_5_4))
EXTRA_CFLAGS += -DLINUX_5_4
endif

ccflags-y := -I $(PWD)/inc
EXTRA_CFLAGS += -g

