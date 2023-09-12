obj-m += kernel_module.o
obj-y += kernel_module.ko

PWD := $(CURDIR)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
load:
	sudo insmod $(obj-y)
unload:
	sudo rmmod $(obj-y)
