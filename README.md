# 16x2 LCD Linux Device Driver
Linux sysfs driver for L1602 LCD module.

Tested using BeagleBone Black and LCD L1602A module.

## Prerequisites
- Linux source for BBB (https://github.com/beagleboard/linux)
- BeagleBone Black Rev C (BBB)
- L1602 LCD module

## LCD connection to BBB
 BBB Connector_Pin | LCD Pin
--- | ---
P8_45 | RW
P8_46 | E
P8_44 | RS
P8_42 | D4
P8_41 | D5
P8_40 | D6
P8_39 | D7

## Build
- Update `KERN_PATH` in `Makefile`.
- Add `#include "am335x-boneblack-lcd1602.dtsi"` in `arch/arm/boot/dts/am335x-boneblack.dtsam335x-boneblack.dts` file in kernel source directory.
- Run `make dtb`.
- Run `make`.

## Copy to target
- Update `REMOTE` in `Makefile`.
- Run `make scp_dtb`.
- Run `make scp`.
- On target, replace existing dtb.

## Load driver module
- On target, run `insmod lcd1602_driver.ko`.
- On target, use sysfs entries in `/sys/class/lcd_ext/lcd_datetime/` directory to interact with the LCD:
    - View available commands: `cat cmd`.
    - Send a command: `echo clear > cmd`.
    - Set cursor location: `echo 40 > cursor` (Cursor location can be in the range 0 to 79, both inclusive).
    - Write text: `echo -n TEST > text`.

## Run test script
- Run `make scp_sh`.
- On target, run `test-lcd.sh` (Prints date and time on LCD every second).

## Unload driver module
- On target, run `rmmod lcd1602_driver`.

## Device Tree customizations
Properties can be changed in `am335x-boneblack-lcd1602.dtsi` to change LCD pin connections and sysfs directory name (`lcd_datetime`).
- `label`: Change to required sysfs directory name.
- `<function>-gpios`: Change to required pins.
- `pinctrl-single,pins`: Update as per the changed `<function>-gpios`.
- `status` property of `tda19988` can be changed, if the updated pins doesnot conflict with that of `tda19988`.
