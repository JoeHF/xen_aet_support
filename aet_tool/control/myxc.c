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
#include <sys/time.h>
#include <time.h>

double thresh_hold = 0.98;
static struct AET_ctrl *aet_ctrl = (struct AET_ctrl *)PML4_ADDR(270ul);
static char* NAME[10] = {"SET", "REVERSED", "USER_MODE", "TLB_COUNTER", "MEM_COUNTER", "DIFF"};

//#ifdef AET_PF
static void aet_process(int dom, unsigned long n) {
    int STEP = 256;
    unsigned long long m = aet_ctrl->node_count_[dom] * STEP;
    unsigned long long tott = aet_ctrl->tot_ref_[dom];
	if (tott == 0)
		return;
    unsigned long long T = 0;
	unsigned long long surplus;
	unsigned long first = aet_ctrl->aet_hist_[dom][1];
	unsigned long longest = aet_ctrl->longest_aet_hist_pos[dom];
	unsigned long aet_hist_[MAX_DOM_NR][MAX_PAGE_NUM];
	memcpy(aet_hist_, aet_ctrl->aet_hist_, sizeof(aet_ctrl->aet_hist_));
	struct timeval start;
	struct timezone tz;
	struct timeval end;
	gettimeofday(&start, &tz);
    int domain = 256;
    double tot = 0;
	char miss_curve_file_name[30];
	FILE *fsz;
    if (n != 0) { 
		fsz	= fopen("miss_curve.txt","w");
		printf("old miss curve file name\n");
	}
	else { 
		sprintf(miss_curve_file_name, "%d_miss_curve.txt", start.tv_sec);
		fsz = fopen(miss_curve_file_name, "w");
		printf("miss curve file name:%s\n", miss_curve_file_name);
	}

	if (n == 0) { 
		n = aet_ctrl->mem_now - aet_ctrl->mem_last;
		aet_ctrl->mem_last = aet_ctrl->mem_now;
	}
	else { 
		n = n - aet_ctrl->mem_last;	
	}
	//modified rtd 1
	surplus /= STEP;

    //double N = tott + m;
    double N = tott + 1.0 * tott / (n-m) * m;
	double sum = 0.0;
	
	printf("endless:%lf\n", 1.0 * tott / (n - m) * m / N);
	printf("m:%lu\n", m);
	printf("n:%lu\n", n);
	printf("tott:%lu\n", tott);
	printf("N:%lf\n", N);
	printf("longest:%lu\n", longest);
	printf("surplus:%lu total:%lu time:%lu\n", surplus, aet_ctrl->surplus_total, aet_ctrl->surplus_time);
	//clear
	aet_ctrl->tot_ref_[dom] = 0;
	aet_ctrl->longest_aet_hist_pos[dom] = 0;
	memset(aet_ctrl->aet_hist_, 0, sizeof(aet_ctrl->aet_hist_));
    unsigned long step = 1;
    int _dom = 0, dT = 0, loc = 0;
    unsigned long c;
    const int PGAP = 256;
    		
    //printf("dom=%d",seq);
    m = 256 * 2000;
	for (c = 1 ; c <= m ; c++) {
        while (T <= n && tot/N < c && N > sum) {
            tot += N - sum;
            T++;
            if (T > loc) {
                if (++_dom > domain) _dom = 1 , step *= 2;
                loc += step;
                dT++;
            }

			if (dT >= MAX_PAGE_NUM) {
				printf("[WARN] dT exceeds MAX_AET_HIST\n");
				break;
			}

			if (dT >= longest) { 
				printf("[INFO] dT exceeds longest aet hist pos\n");
				break;
			}

			if ((sum - (double)first) / (double)(tott - first) > thresh_hold) { 
				printf("[INFO] miss rate:%6lf exceeds the maximum\n", (sum - (double)first) / (double)(tott - first));
				break;
			}

            sum += 1.0 * aet_hist_[dom][dT] / step;
        }
        //ans[c] = 1.0*(N-sum)/N;

		if (dT >= MAX_PAGE_NUM) {
			printf("dt exceeds max aet hist\n");
			break;
		}

		if (dT >= longest) { 
			printf("[INFO] dT exceeds longest aet hist pos\n");
			break;
		}

		if ((sum - (double)first) / (double)(tott - first) > thresh_hold) { 
			printf("[INFO] miss rate:%6lf exceeds the maximum\n", (sum - (double)first) / (double)(tott - first));
			break;
		}

        if (c % PGAP == 0) {
			if ((c / PGAP) % 50 == 0) { 
				printf("mem:%d finished %6lf/%lu rate:%6lf, dT now:%d\n", c / PGAP, sum - (double)first, tott - first, (sum - (double)first) / (double)(tott - first), dT);
			}

			fprintf(fsz, "dT:%d sum:%lf\n", dT, sum);
			if (sum <= N) {
				fprintf(fsz, "%.6lf\n", 1.0 * (N - sum) / N);
			}
			else {
				//printf("weird N:%lf sum:%lf\n", N, sum);
				fprintf(fsz, "0\n");
			}
		}
    }
    fclose(fsz);
	gettimeofday(&end, &tz);
	unsigned  long diff;
	diff = (end.tv_sec-start.tv_sec);
	printf("aet run time:%lus\n", diff);
	
}
//#endif
void print(int arg) {
	if (arg != 2) { 
		printf("start:%d mfn:%lx track:%d open:%d\n", aet_ctrl->start_, aet_ctrl->sl4mfn_, aet_ctrl->track_, aet_ctrl->open_);
		printf("reversed_aet_magic_count/set_aet_magic_count: %lu/%lu tracked_aet_magic_count/set_aet_magic_count: %lu/%lu tracked_aet_magic_count/set_aet_magic_count: %lu/%lu\n", 
				aet_ctrl->reversed_aet_magic_count, aet_ctrl->set_aet_magic_count, aet_ctrl->tracked_aet_magic_count1, aet_ctrl->set_aet_magic_count, aet_ctrl->tracked_aet_magic_count2, aet_ctrl->set_aet_magic_count);
		printf("page fault count:%llu\n", aet_ctrl->page_fault_count);
		printf("user mode:%lu reserved bit:%lu both:%lu\n", aet_ctrl->user_mode_fault, aet_ctrl->reserved_bit_fault, aet_ctrl->both_fault);
		printf("total count:%d set_pending_page_num:%lu all_sl1mfn_num:%d set sl1mfn page num:%lu\n", aet_ctrl->total_count, aet_ctrl->set_pending_page_num, aet_ctrl->sl1mfn_num, aet_ctrl->set_sl1mfn_page_num);
		printf("hash conflict:%llu vmexit_num:%lu\n", aet_ctrl->hash_conflict_num, aet_ctrl->vmexit_num);
		printf("valid sl1mfn:%lu \n", aet_ctrl->valid_sl1mfn[0]);
	}
	
	if (arg == 1) {
		FILE *fp;
		fp = fopen("va.txt", "w");

		int i;
		for (i = 0 ; i < aet_ctrl->total_count ; i++) {
			fprintf(fp, "va:%lx type:%s vmexit num:%lu %lx ec:%lu mem_counter:%lu\n", aet_ctrl->tvs[i].va, NAME[aet_ctrl->tvs[i].type], aet_ctrl->tvs[i].l1p, aet_ctrl->tvs[i].l1, aet_ctrl->tvs[i].ec, aet_ctrl->tvs[i].mc);	
		}

		fclose(fp);
	}

	if (arg == 2) {
		FILE *fp1, *fp2;
		int i, j;
		fp1 = fopen("hash.txt", "w");
		for (i = 0 ; i < HASH ; i++) {
			for (j = 0 ; j < HASH_CONFLICT_NUM ; j++) {
				if (aet_ctrl->hns_[0][i][j].mfn != 0) {
					fprintf(fp1, "i:%d j:%d mfn:%lx mem_counter:%lu\n", i, j, aet_ctrl->hns_[0][i][j].mfn, aet_ctrl->hns_[0][i][j].mem_counter);
				}
			}
		}

		fp2 = fopen("aet_hist.txt", "w");
		for (i = 0 ; i < MAX_PAGE_NUM ; i++) {
			fprintf(fp2, "i:%d hist:%lu\n", i, aet_ctrl->aet_hist_[0][i]);
		}

		fclose(fp2);
	}
	
}

void reset() {
	aet_ctrl->total_count = 0;
	aet_ctrl->surplus_total = 0;
	aet_ctrl->surplus_time = 0;
	aet_ctrl->set_aet_magic_count = 0;
	aet_ctrl->tracked_aet_magic_count1 = 0;
	aet_ctrl->tracked_aet_magic_count2 = 0;
	aet_ctrl->reversed_aet_magic_count = 0;
	aet_ctrl->page_fault_count = 0;
	aet_ctrl->user_mode_fault = 0;
	aet_ctrl->reserved_bit_fault = 0;
	aet_ctrl->both_fault = 0;
	aet_ctrl->hash_conflict_num = 0;
	aet_ctrl->set_pending_page_num = 0;
	aet_ctrl->sl1mfn_num = 0;
	aet_ctrl->set_sl1mfn_page_num = 0;
	aet_ctrl->set_num = 0;
	aet_ctrl->vmexit_num = 0;
	aet_ctrl->mem_now = 0;
	aet_ctrl->mem_last = 0;
	memset(aet_ctrl->valid_sl1mfn, 0, sizeof(aet_ctrl->valid_sl1mfn));
	//memset(aet_ctrl->valid_node_count, 0, sizeof(aet_ctrl->valid_node_count));
	memset(aet_ctrl->hns_, 0, sizeof(aet_ctrl->hns_));
	memset(aet_ctrl->aet_hist_, 0, sizeof(aet_ctrl->aet_hist_));
	memset(aet_ctrl->longest_aet_hist_pos, 0, sizeof(aet_ctrl->longest_aet_hist_pos));
	memset(aet_ctrl->node_count_, 0, sizeof(aet_ctrl->node_count_));
	memset(aet_ctrl->tot_ref_, 0, sizeof(aet_ctrl->tot_ref_));
	memset(aet_ctrl->lps, 0, sizeof(aet_ctrl->lps));
	memset(aet_ctrl->tvs, 0, sizeof(aet_ctrl->tvs));
	memset(aet_ctrl->pds, 0, sizeof(aet_ctrl->pds));
	memset(aet_ctrl->all_sl1mfn, 0, sizeof(aet_ctrl->all_sl1mfn));
}

int main(int argc, char** argv) {
	int ch;
	while ((ch = getopt(argc, argv, "s:rc:")) != -1) {
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
			case 'c':
				printf("option c:%s\n", optarg);
				unsigned long n = strtoul(optarg, NULL, 0);
				aet_process(0, n);
			default:
				break;
		}
	}


	return 0;
}
