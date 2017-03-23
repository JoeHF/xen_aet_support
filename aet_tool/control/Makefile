all: xc

xc: myxc.c mytypes.h /usr/local/include/xen/aet.h
	gcc -lm -O3 -DDOM0 -lxenctrl -o $@ $<
