##
## Makefile
##
## Made by xsyann
## Contact <contact@xsyann.com>
##
## Started on  Fri May  9 11:31:36 2014 xsyann
## Last update Fri May  9 12:16:43 2014 xsyann
##

TARGET	= netmalloc

obj-m	+= $(TARGET).o

$(TARGET)-objs := src/netmalloc.o src/syscall.o

CURRENT = $(shell uname -r)
KDIR	= /lib/modules/$(CURRENT)/build

all	:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean	:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load	:
	@sudo rmmod ./$(TARGET).ko 2> /dev/null || true
	@sudo insmod ./$(TARGET).ko
	@echo "Load $(TARGET).ko"

unload	:
	@sudo rmmod ./$(TARGET).ko
	@echo "Unload $(TARGET).ko"
