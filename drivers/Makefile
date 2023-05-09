ifndef PWD
$(error PWD variable is not set; Please check README!!!)
endif

include mk/common.mk

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