make xen && make tools && make install-xen && make install-tools && rm -f /usr/local/include/xen/aet.h && cp xen/include/public/aet.h /usr/local/include/xen
if [ $? -eq 0 ];then
	ctags -R
	cd /houfang/xen-4.5.1/aet_tool/control
	make
	xl destroy 1
	reboot
fi	
