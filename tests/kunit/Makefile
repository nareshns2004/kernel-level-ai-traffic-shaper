ifneq ($(KERNELRELEASE),)
include $(src)/Kbuild
else
KDIR ?= /lib/modules/$(shell uname -r)/build

.PHONY: default modules clean
default: modules

modules:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
endif
