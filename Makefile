obj-m := lcd1602_driver.o

lcd1602_driver-objs := driver.o lcd.o gpio.o

ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-
KERN_DIR=/home/ra/workspace/ldd/source/linux-6.1.46-ti-r18

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) clean

help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) help

scp:
	scp *.ko debian@192.168.5.2:~/

scp_dtb:
	scp $(KERN_DIR)/arch/arm/boot/dts/am335x-boneblack.dtb debian@192.168.5.2:~/

scp_sh:
	scp *.sh debian@192.168.5.2:~/
