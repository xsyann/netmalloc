##
## Makefile
##
## Made by xsyann
## Contact <contact@xsyann.com>
##
## Started on  Fri May  9 11:31:36 2014 xsyann
## Last update Sun May 25 16:27:18 2014 xsyann
##

TARGET	= netmalloc

obj-m	+= $(TARGET).o

SRC = src

$(TARGET)-objs := $(SRC)/netmalloc.o $(SRC)/syscall.o \
		  $(SRC)/vma.o $(SRC)/area.o $(SRC)/storage.o \
		  $(SRC)/generic_malloc.o $(SRC)/stored_page.o \
		  $(SRC)/mapped_buffer.o $(SRC)/socket.o

CURRENT = $(shell uname -r)
KDIR	= /lib/modules/$(CURRENT)/build

ARGS = "192.168.69.1:12345"

all	:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean	:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load	:
	@sudo rmmod ./$(TARGET).ko 2> /dev/null || true
	@sudo insmod ./$(TARGET).ko server=${ARGS}
	@echo "Load $(TARGET).ko"

unload	:
	@sudo rmmod ./$(TARGET).ko
	@echo "Unload $(TARGET).ko"
