HOME = /opt/iot-devkit/1.7.3/sysroots
KDIR:=$(HOME)/i586-poky-linux/usr/src/kernel  
PATH := $(PATH):$(HOME)/x86_64-pokysdk-linux/usr/bin/i586-poky-linux
CROSS_COMPILE = i586-poky-linux-
SROOT=$(HOME)/i586-poky-linux/
CC = i586-poky-linux-gcc
ARCH = x86
APP = hscr_tester

obj-m:= i2c_flash.o

all:

	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

	$(CC) -o $(APP) main.c -Wall -g --sysroot=$(SROOT)

clean:

	rm -f *.ko

	rm -f *.o

	rm -f Module.symvers

	rm -f modules.order

	rm -f *.mod.c

	rm -rf .tmp_versions

	rm -f *.mod.c

	rm -f *.mod.o

	rm -f \.*.cmd

	rm -f Module.markers

	rm -f $(APP) 
