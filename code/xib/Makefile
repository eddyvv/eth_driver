KERNELDIR_ := /lib/modules/$(shell uname -r)/build

obj-m += xib.o

xib-objs := main.o

all:
	$(MAKE) -C $(KERNELDIR_) M=$(CURDIR) modules

clean:
	make -C $(KERNELDIR_) M=$(CURDIR) clean