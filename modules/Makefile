obj-m = changeParent.o

KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
	sudo insmod changeParent.ko

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
	sudo rmmod changeParent
