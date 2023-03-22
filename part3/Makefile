obj-y := sys_calls.o
obj-m := bar.o
PWD := $(shell pwd)
KDIR := /lib/modules/`uname -r`/build
default:
        $(MAKE) -C $(KDIR) M=$(PWD) SUBDIRS=$(PWD) modules

clean:
        rm -f *.o *.ko *.mod.* Module.* modules.* *.mod .*.cmd
