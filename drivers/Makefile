ifndef PWD
$(error PWD variable is not set; Please check README!!!)
endif

ifndef PLATFORM
$(error PLATFORM variable is not set; Please check README!!!)
endif
include mk/common.mk
SUBDIRS_ALL := eth xib

ifeq ($(PLATFORM), $(KERNEL_VERSION_5_10_VM))
$(info PLATFORM = $(PLATFORM))
else ifeq ($(PLATFORM), $(KERNEL_VERSION_5_10))
$(info PLATFORM = $(PLATFORM))
else
$(error PLATFORM setting error; Please check README!!!)
endif

.PHONY: all clean install install_xib install_eth $(SUBDIRS_ALL)

all: $(SUBDIRS_ALL)
$(SUBDIRS_ALL):
	make -C $@ all

install_eth:
	make eth
ifeq ($(PLATFORM),$(KERNEL_VERSION_5_10_VM))
	sudo rmmod e1000
endif
	sudo insmod ./eth/xtic_nic.ko

install:
	make all
ifeq ($(PLATFORM),$(KERNEL_VERSION_5_10_VM))
	sudo rmmod e1000
endif
	sudo insmod ./eth/xtic_nic.ko
	sudo insmod ./xib/xib.ko

ifeq ($(PLATFORM),$(KERNEL_VERSION_5_10_VM))
install_e1000:
	sudo insmod /lib/modules/$(shell uname -r)/kernel/drivers/net/ethernet/intel/e1000/e1000.ko
endif

install_ib_core:
	sudo insmod /lib/modules/$(shell uname -r)/kernel/drivers/infiniband/core/ib_core.ko

printenv:
	printenv PLATFORM PWD

clean:
	@for dir in $(SUBDIRS_ALL); do \
		make -C $$dir clean; \
	done
	sudo rmmod xib xtic_nic
ifeq ($(PLATFORM),$(KERNEL_VERSION_5_10_VM))
	sudo insmod /lib/modules/$(shell uname -r)/kernel/drivers/net/ethernet/intel/e1000/e1000.ko
endif