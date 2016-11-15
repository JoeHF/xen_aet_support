all: xc

xc: myxc.c mytypes.h /usr/local/include/xen/aet.h
	gcc -O3 -DDOM0 -lxenctrl -o $@ $<
