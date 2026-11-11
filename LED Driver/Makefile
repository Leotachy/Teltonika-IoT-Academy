obj-m += blinky_led_drv.o
blinky_led_drv-y := blinky_led.o dt_parse.o

BUILD_DIR := $(PWD)/build

all:
	make -C /home/andrijauskas.lu/Desktop/raspberry_pi_OS/linux-6.18.56 M=$(PWD) MO=$(BUILD_DIR) modules
clean:
	make -C /home/andrijauskas.lu/Desktop/raspberry_pi_OS/linux-6.18.56 M=$(PWD) MO=$(BUILD_DIR) clean
