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
static char* NAME[3] = {"SET", "REVERSED", "USER_MODE"};

void print(int arg) {
	printf("start:%d mfn:%lx track:%d open:%d\n", aet_ctrl->start_, aet_ctrl->sl4mfn_, aet_ctrl->track_, aet_ctrl->open_);
	printf("reversed_aet_magic_count/set_aet_magic_count: %lu/%lu tracked_aet_magic_count/set_aet_magic_count: %lu/%lu tracked_aet_magic_count/set_aet_magic_count: %lu/%lu\n", 
			aet_ctrl->reversed_aet_magic_count, aet_ctrl->set_aet_magic_count, aet_ctrl->tracked_aet_magic_count1, aet_ctrl->set_aet_magic_count, aet_ctrl->tracked_aet_magic_count2, aet_ctrl->set_aet_magic_count);
	printf("page fault count:%llu\n", aet_ctrl->page_fault_count);
	printf("user mode:%lu reserved bit:%lu both:%lu\n", aet_ctrl->user_mode_fault, aet_ctrl->reserved_bit_fault, aet_ctrl->both_fault);
	printf("total count:%d\n", aet_ctrl->total_count);
	
	if (arg == 1) {
		FILE *fp;
		fp = fopen("va.txt", "w");

		int i;
		for (i = 0 ; i < aet_ctrl->total_count ; i++) {
			fprintf(fp, "va:%lx type:%s l1:%lx %lx\n", aet_ctrl->tvs[i].va, NAME[aet_ctrl->tvs[i].type], aet_ctrl->tvs[i].l1p, aet_ctrl->tvs[i].l1);	
		}

		fclose(fp);
	}
	
}

void reset() {
	aet_ctrl->total_count = 0;
	aet_ctrl->set_aet_magic_count = 0;
	aet_ctrl->tracked_aet_magic_count1 = 0;
	aet_ctrl->tracked_aet_magic_count2 = 0;
	aet_ctrl->reversed_aet_magic_count = 0;
	aet_ctrl->page_fault_count = 0;
	aet_ctrl->user_mode_fault = 0;
	aet_ctrl->reserved_bit_fault = 0;
	aet_ctrl->both_fault = 0;
}

int main(int argc, char** argv) {
	int ch;
	while ((ch = getopt(argc, argv, "s:r")) != -1) {
		switch (ch) {
			case 's':
				printf("option s:%s\n", optarg);
				int arg = atoi(optarg);
				print(arg);
				break;
			case 'r':
				printf("option r\n");
				reset();
				break;
			default:
				break;
		}
	}


	return 0;
}
