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



SUBDIRS_ALL := eth xib

.PHONY: all clean install install_xib install_eth $(SUBDIRS_ALL)

all: $(SUBDIRS_ALL)

$(SUBDIRS_ALL):
	make -C $@ all

install_xib:
	make xib
	sudo insmod ./xib/xib.ko

install_eth:
	make eth
	sudo insmod ./eth/xtic_nic.ko

install:
	make all
	sudo insmod ./eth/xtic_nic.ko
	sudo insmod ./xib/xib.ko

clean:
	@for dir in $(SUBDIRS_ALL); do \
		make -C $$dir clean; \
	done
	sudo rmmod xib xtic_nic