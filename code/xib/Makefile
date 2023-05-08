KERNELDIR_ := /lib/modules/$(shell uname -r)/build

obj-m += xilinx_ib.o

xilinx_ib-objs := main.o

all:
	$(MAKE) -C $(KERNELDIR_) M=$(CURDIR) modules

clean:
	make -C $(KERNELDIR_) M=$(CURDIR) clean