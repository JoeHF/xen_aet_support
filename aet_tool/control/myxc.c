#include <stdio.h>
#include <stdlib.h>
#include <xenctrl.h>
#include <xen/sys/privcmd.h>
#include <linux/mman.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include "mytypes.h"
#include <xen/aet.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

static struct AET_ctrl *aet_ctrl = (struct AET_ctrl *)PML4_ADDR(270ul);

int main(int argc, char** argv) {
	printf("start:%d mfn:%lx track:%d\n", aet_ctrl->start_, aet_ctrl->sl4mfn_, aet_ctrl->track_);
	return 0;
}
