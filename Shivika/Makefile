obj-m+=leddriver.o
 
all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	$(CC) -lrt userspace.c -o userspace -lm -lpthread
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm userspace
