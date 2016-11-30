obj-m := drtest.o
kernel_path=/houfang/research/linux-4.2.1
all:
	make -C $(kernel_path) M=$(PWD) modules
clean:
	make -C $(kernel_path) M=$(PWD) clean

