obj-m := lcd1602_driver.o

lcd1602_driver-objs := src/driver.o src/lcd.o src/gpio.o

ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-
KERN_DIR=/home/ra/workspace/ldd/source/linux-6.1.46-ti-r18
REMOTE=debian@192.168.5.2

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) clean

help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) help	

dtb:
	cp *.dtsi $(KERN_DIR)/arch/arm/boot/dts/
	make -C $(KERN_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) am335x-boneblack.dtb

scp:
	scp *.ko $(REMOTE):~/

scp_dtb:
	scp $(KERN_DIR)/arch/arm/boot/dts/am335x-boneblack.dtb $(REMOTE):~/

scp_sh:
	scp *.sh $(REMOTE):~/
