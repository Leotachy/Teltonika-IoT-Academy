obj-m += my_lcd_drv.o
my_lcd_drv-y := my_lcd.o simple_string.o i2c-lcd.o lt_lcd_letter.o

BUILD_DIR := $(PWD)/build

all:
	make -C /home/andrijauskas.lu/Desktop/raspberry_pi_OS/linux-6.18.56 M=$(PWD) MO=$(BUILD_DIR) modules
clean:
	make -C /home/andrijauskas.lu/Desktop/raspberry_pi_OS/linux-6.18.56 M=$(PWD) MO=$(BUILD_DIR) clean
