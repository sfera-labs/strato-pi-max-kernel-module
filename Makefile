obj-m += stratopimax.o

stratopimax-objs := module.o
stratopimax-objs += commons/commons.o
stratopimax-objs += gpio/gpio.o
stratopimax-objs += atecc/atecc.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

install:
	sudo install -m 644 -c stratopimax.ko /lib/modules/$(shell uname -r)
	sudo depmod
