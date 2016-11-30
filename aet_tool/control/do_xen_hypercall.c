#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <xenctrl.h>
#include <xen/sys/privcmd.h>
enum AET_CMD
{
	INVALID=0, PMU_CMD, AET_CMD
};
int main(int argc, char *argv[])
{
    int fd, ret;
	enum AET_CMD aet_cmd = PMU_CMD;
    long arg0, arg1, arg2, arg3;
    if (argc != 5) {
        printf("please put four parameters!\n");
        return -1;
    }
	arg0 = atol(argv[1]);
    arg1 = atol(argv[2]);
    arg2 = atol(argv[3]);
    arg3 = atol(argv[4]);
    privcmd_hypercall_t hcall = {
        __HYPERVISOR_xc_reserved_op,
        {arg0, arg1, arg2, arg3, 0}
    };
    fd = open("/proc/xen/privcmd", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(1);
    } else
//        printf("fd = %d\n", fd);
    ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hcall);
//    printf("ret = %d\n", ret);
}
