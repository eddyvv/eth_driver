ifndef PWD
$(error PWD variable is not set; Please check README!!!)
endif

include mk/common.mk
SUBDIRS_ALL := eth xib

.PHONY: all clean install install_xib install_eth $(SUBDIRS_ALL)

all: $(SUBDIRS_ALL)
$(SUBDIRS_ALL):
	make -C $@ all

install_eth:
	make eth
ifeq ($(CURRENT_KERNEL_VERSION),$(KERNEL_VERSION_5_15))
	sudo rmmod e1000
endif
	sudo insmod ./eth/xtic_nic.ko

install:
	make all
ifeq ($(CURRENT_KERNEL_VERSION),$(KERNEL_VERSION_5_15))
	sudo rmmod e1000
endif
	sudo insmod /lib/modules/$(shell uname -r)/kernel/drivers/infiniband/core/ib_core.ko
	sudo insmod ./eth/xtic_nic.ko
	sudo insmod ./xib/xib.ko

ifeq ($(CURRENT_KERNEL_VERSION),$(KERNEL_VERSION_5_15))
install_e1000:
	sudo insmod /lib/modules/$(shell uname -r)/kernel/drivers/net/ethernet/intel/e1000/e1000.ko
endif

clean:
	@for dir in $(SUBDIRS_ALL); do \
		make -C $$dir clean; \
	done
	sudo rmmod xib xtic_nic
	sudo rmmod /lib/modules/$(shell uname -r)/kernel/drivers/infiniband/core/ib_core.ko
ifeq ($(CURRENT_KERNEL_VERSION),$(KERNEL_VERSION_5_15))
	sudo insmod /lib/modules/$(shell uname -r)/kernel/drivers/net/ethernet/intel/e1000/e1000.ko
endif