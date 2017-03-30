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
#include <math.h>

double thresh_hold = 1;
int limit = 2000;
static struct AET_ctrl *aet_ctrl = (struct AET_ctrl *)PML4_ADDR(270ul);
static char* NAME[10] = {"SET", "REVERSED", "USER_MODE", "TLB_COUNTER", "MEM_COUNTER", "DIFF"};

struct timeval start;
struct timezone tz;
struct timeval end;

//#ifdef AET_PF
static void lru_process(unsigned long lru_hist_[], unsigned long cold_miss) { 
	double total = 0;
	FILE *lru_mcf;
	char lru_miss_curve_name[20];
	total += (double)cold_miss; 
	for (int i = 0 ; i < LRU_MAX_PAGE_NUM ; i++) 
		total += (double)lru_hist_[i];
	printf("lru endless:%.10lf\n", (double)cold_miss / total);
	sprintf(lru_miss_curve_name, "%d_lru_mc.txt", start.tv_sec);
	lru_mcf = fopen(lru_miss_curve_name, "w");

	double sum = 0.0;
	int m = limit * 256;
	for (int i = 0 ; i < m ; i++) { 
		sum += (double)lru_hist_[i];
		fprintf(lru_mcf, "%.10lf\n", 1.0 - sum / total);
	}

	fclose(lru_mcf);
}

static void set_track_rate(int track_rate) { 
	printf("option old track_rate:%d new track_rate:%d\n", aet_ctrl->track_rate, track_rate);	
	aet_ctrl->track_rate = track_rate;
	//aet_ctrl->hot_set_size = DEFAULT_HOT_SET_SIZE / aet_ctrl->track_rate;
	printf("hot set size:%d\n", aet_ctrl->hot_set_size);
}

static void dynamic_track_rate(unsigned long mem_diff, unsigned long pf_diff) { 
	printf("dynamic track rate mem diff:%lu pf diff:%lu pf ps:%lu\n", mem_diff, pf_diff, pf_diff / 10);
	unsigned long mem_pf_rate = mem_diff / pf_diff;
	unsigned long pf_per_s = pf_diff / 10;
	int r;
	int upper_bound = 5000;
	int lower_bound = 1000;
	if (pf_per_s > upper_bound) { 
		r = (int)(log(pf_per_s / upper_bound) / log(2));
		set_track_rate(aet_ctrl->track_rate * pow(2, r));
		aet_ctrl->reset = 0;
	}
	else if (pf_per_s < 3000) { 
		set_track_rate(aet_ctrl->track_rate / 2);
		aet_ctrl->reset = 0;
	}

}

static void aet_process(int dom, unsigned long n, int aet_time) {
	gettimeofday(&start, &tz);

    unsigned long long m = aet_ctrl->node_count_[dom] * aet_ctrl->track_rate;
    unsigned long long tott = aet_ctrl->tot_ref_[dom];
	if (aet_ctrl->reset == 1)
		aet_ctrl->reset = 0;
	
	/*
	if (aet_time >= 30) { 
		aet_ctrl->sleep = 1;
		//printf("cal aet time 30 times exits! tott:%d\n", tott);
		return;
	}
	*/
	
	unsigned long long T = 0;
	unsigned long first = aet_ctrl->aet_hist_[dom][1];
	int longest = aet_ctrl->longest_aet_hist_pos[dom];
	unsigned long aet_hist_[MAX_DOM_NR][MAX_PAGE_NUM];
	unsigned long lru_hist_[LRU_MAX_PAGE_NUM];
	unsigned long cold_miss = aet_ctrl->cold_miss_[dom];
	unsigned long lru_cold_miss = aet_ctrl->lru_cold_miss_[dom];
	memcpy(aet_hist_, aet_ctrl->aet_hist_, sizeof(aet_ctrl->aet_hist_));
	memcpy(lru_hist_, aet_ctrl->lru_hist_, sizeof(aet_ctrl->lru_hist_));
	//clear
	aet_ctrl->tot_ref_[dom] = 0;
	aet_ctrl->cold_miss_[dom] = 0;
	aet_ctrl->lru_cold_miss_[dom] = 0;
	aet_ctrl->longest_aet_hist_pos[dom] = 0;
	memset(aet_ctrl->aet_hist_, 0, sizeof(aet_ctrl->aet_hist_));
	memset(aet_ctrl->lru_hist_, 0, sizeof(aet_ctrl->lru_hist_));
	unsigned long mem_diff = aet_ctrl->mem_now - aet_ctrl->mem_last;
	unsigned long pf_diff = aet_ctrl->track_fault_time - aet_ctrl->track_fault_time_last;
	aet_ctrl->mem_last = aet_ctrl->mem_now;
	aet_ctrl->track_fault_time_last = aet_ctrl->track_fault_time;
	char miss_curve_file_name[30];
	char aet_hist_file_name[30];
	char lru_hist_file_name[30];
	char sl1mfn_file_name[30];
	FILE *mcf, *aef, *lruf;
	//FILE *sl1mfnf;
	sprintf(aet_hist_file_name, "%d_aet_hist.txt", start.tv_sec);
	sprintf(lru_hist_file_name, "%d_lru_hist.txt", start.tv_sec);
	sprintf(sl1mfn_file_name, "sl1mfn_%d.txt", start.tv_sec);
	aef = fopen(aet_hist_file_name, "w");
	lruf = fopen(lru_hist_file_name, "w");
	//sl1mfnf = fopen(sl1mfn_file_name, "w");
	/*
	for (int i = 0 ; i < HASH ; i++) { 
		for (int j = 0 ; j < HASH_CONFLICT_NUM ; j++) { 
			if (aet_ctrl->all_sl1mfn_hash[i][j].sl1mfn != 0)
				fprintf(sl1mfnf, "i:%d j:%d %lu\n", i, j, aet_ctrl->all_sl1mfn_hash[i][j].sl1mfn);	
		}
	}
	*/
	/*
	for (int i = aet_ctrl->sl1mfn_num - 1 ; i >= 0 ; i--) { 
		fprintf(sl1mfnf, "i:%d %lu\n", i, aet_ctrl->all_sl1mfn[i].sl1mfn);
	}
	*/
	//fclose(sl1mfnf);
	/*	
	if (aet_time % 3 == 0)
		aet_ctrl->reset = 0;
	*/	
	if (tott + cold_miss < 100) { 
		aet_ctrl->reset = 0;
		printf("------\n");
		printf("tott is zero reset:%d sl1mfn_num:%d last_set_num:%d tott:%lu cold miss:%lu\n", aet_ctrl->reset, aet_ctrl->sl1mfn_num, aet_ctrl->last_set_num, tott, cold_miss);
		return;
	}

	printf("------\n");
	//dynamic_track_rate(mem_diff, pf_diff);
	/*
	if (aet_ctrl->sleep == 0) { 
		printf("sl1mfn_num:%d last_set_num:%d sleep:%d\n", aet_ctrl->sl1mfn_num, aet_ctrl->last_set_num, aet_ctrl->sleep);
		//aet_ctrl->sleep = 1;
	}
	else { 
		// force reset if running enough time
		if (aet_time % 4 == 0) { 
			aet_ctrl->sleep = 0;
		}
		return;
	}
	*/
    
	/*
	char miss_curve_file_name[30];
	char aet_hist_file_name[30];
	char lru_hist_file_name[30];
	char sl1mfn_file_name[30];
	FILE *mcf, *aef, *lruf;
	FILE *sl1mfnf;
	sprintf(aet_hist_file_name, "%d_aet_hist.txt", start.tv_sec);
	sprintf(lru_hist_file_name, "%d_lru_hist.txt", start.tv_sec);
	sprintf(sl1mfn_file_name, "sl1mfn_%d.txt", start.tv_sec);
	aef = fopen(aet_hist_file_name, "w");
	lruf = fopen(lru_hist_file_name, "w");
	sl1mfnf = fopen(sl1mfn_file_name, "w");
	for (int i = aet_ctrl->sl1mfn_num - 1 ; i >= 0 ; i--) { 
		fprintf(sl1mfnf, "i:%d %lu\n", i, aet_ctrl->all_sl1mfn[i]);
	}
	fclose(sl1mfnf);
	*/

    if (n != 0) { 
		mcf	= fopen("miss_curve.txt","w");
		printf("old miss curve file name\n");
	}
	else { 
		sprintf(miss_curve_file_name, "%d_miss_curve.txt", start.tv_sec);
		mcf = fopen(miss_curve_file_name, "w");
		//printf("miss curve file name:%s\n", miss_curve_file_name);
	}

	for (int i = 0 ; i < MAX_PAGE_NUM ; i++) { 
		fprintf(aef, "%lu\n", aet_hist_[dom][i]);
	}

	for (int i = 0 ; i < LRU_MAX_PAGE_NUM ; i++) { 
		fprintf(lruf, "%lu\n", lru_hist_[i]);
	}

	if (n == 0) { 
		n = aet_ctrl->mem_now - aet_ctrl->mem_last;
		aet_ctrl->mem_last = aet_ctrl->mem_now;
	}
	else { 
		n = n - aet_ctrl->mem_last;	
	}

	/*
	unsigned long dtlb_miss = aet_ctrl->dtlb_miss_now - aet_ctrl->dtlb_miss_last;
	aet_ctrl->dtlb_miss_last = aet_ctrl->dtlb_miss_now;

	//modified rtd 1
	unsigned long surplus = (n - dtlb_miss) / TRACK_RATE;
	aet_hist_[dom][1] += surplus;
	tott += surplus;
	*/

    //double N = tott + m;
    //double N = tott + 1.0 * tott / (n-m) * m;
	//double N = (double)tott + (double)cold_miss;
	
	printf("start:%d\n", start.tv_sec);
	printf("lru_list_pos:%d lru cold miss:%lu\n", aet_ctrl->lru_list_pos, lru_cold_miss);
	printf("sl1mfn_num:%d last_set_num:%d sleep:%d\n", aet_ctrl->sl1mfn_num, aet_ctrl->last_set_num, aet_ctrl->sleep);
	lru_process(lru_hist_, lru_cold_miss);
	printf("tott:%lu hot set time:%d add hot set num:%lu cold miss:%lu endless:%.10lf\n", tott, aet_ctrl->hot_set_time, aet_ctrl->add_to_hot_set_num, cold_miss, (double)cold_miss / ((double)tott + (double)cold_miss));

	double N = (double)tott;
	int domain = 256;
    double tot = 0.0;
	double sum = 0.0;
    unsigned long step = 1;
    int _dom = 0, dT = 0, loc = 0;
    unsigned long c;
    const int PGAP = 256;
    		
    //printf("dom=%d",seq);
    m = 256 * limit;
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

			/*
			if (dT >= longest) { 
				printf("[INFO] dT:%d exceeds longest aet hist pos\n", dT);
				break;
			}
			*/

			/*
			if ((sum - (double)surplus) / (double)(tott - surplus) > thresh_hold) { 
				printf("[INFO] dT:%d exceeds longest aet hist pos\n", dT);
				printf("[INFO] miss rate:%6lf exceeds the maximum\n", (sum - (double)surplus) / (double)(tott - surplus));
				break;
			}
			*/

            sum += 1.0 * aet_hist_[dom][dT] / step;
        }
        //ans[c] = 1.0*(N-sum)/N;

		if (dT >= MAX_PAGE_NUM) {
			break;
		}

		/*
		if (dT >= longest) { 
			break;
		}
		*/

		/*
		if ((sum - (double)first) / (double)(tott - first) > thresh_hold) { 
			break;
		}
		*/

        if (c % PGAP == 0) {
			if ((c / PGAP) % 200 == 0) { 
				//printf("mem:%d finished %.10lf/%lu rate:%.10lf, dT now:%d\n", c / PGAP, sum - (double)first, tott - first, (sum - (double)first) / (double)(tott - first), dT);
				//printf("%.10lf\n", 1.0 * (N - sum) / N);
			}

			fprintf(mcf, "dT:%d sum:%.10lf\n", dT, sum);
			if (sum <= N) {
				fprintf(mcf, "%.10lf\n", 1.0 * (N - sum) / N);
			}
			else {
				//printf("weird N:%lf sum:%lf\n", N, sum);
				fprintf(mcf, "0\n");
			}
		}
    }

    fclose(mcf);
	gettimeofday(&end, &tz);
	unsigned  long diff;
	diff = (end.tv_sec-start.tv_sec);
	printf("aet run time:%lus aet_time:%d\n", diff, aet_time);
	/*
	if (aet_time % 1 == 0 && aet_ctrl->reset != 3) { 
		printf("reset:%d!!!\n", aet_ctrl->reset);
		aet_ctrl->reset = 0;
	}
	*/
}
//#endif
void print(int arg) {
	if (arg != 2) { 
		printf("start:%d mfn:%lx track:%d open:%d\n", aet_ctrl->start_, aet_ctrl->sl4mfn_, aet_ctrl->track_, aet_ctrl->open_);
		printf("reversed_aet_magic_count/set_aet_magic_count: %lu/%lu tracked_aet_magic_count/set_aet_magic_count: %lu/%lu\n", 
				aet_ctrl->reversed_aet_magic_count, aet_ctrl->set_aet_magic_count, aet_ctrl->tracked_aet_magic_count, aet_ctrl->set_aet_magic_count);
		printf("page fault count:%llu original pf:%llu aet pf:%llu\n", aet_ctrl->page_fault_count, aet_ctrl->page_fault_count - aet_ctrl->tracked_aet_magic_count, aet_ctrl->tracked_aet_magic_count);
		printf("user mode:%lu reserved bit:%lu both:%lu\n", aet_ctrl->user_mode_fault, aet_ctrl->reserved_bit_fault, aet_ctrl->both_fault);
		printf("total count:%d set_pending_page_num:%lu all_sl1mfn_num:%d set sl1mfn page num:%lu\n", aet_ctrl->total_count, aet_ctrl->set_pending_page_num, aet_ctrl->sl1mfn_num, aet_ctrl->set_sl1mfn_page_num);
		printf("hash conflict1:%llu hash conflict2:%llu vmexit_num:%lu\n", aet_ctrl->hash_conflict_num1, aet_ctrl->hash_conflict_num2, aet_ctrl->vmexit_num);
		printf("add to all sl1mfn time:%lus track aet time:%lus rand_track_time:%lus rand_track_num:%lu add_to_all_sl1mfn fail:%d\n", aet_ctrl->add_to_all_sl1mfn_time / 1000000, aet_ctrl->track_aet_time / 10000000, aet_ctrl->rand_track_time / 1000000, aet_ctrl->rand_track_num, aet_ctrl->add_to_sl1mfn_fail);
		printf("sample rate:%d\n", aet_ctrl->track_rate);
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
	aet_ctrl->tracked_aet_magic_count = 0;
	aet_ctrl->reversed_aet_magic_count = 0;
	aet_ctrl->page_fault_count = 0;
	aet_ctrl->user_mode_fault = 0;
	aet_ctrl->reserved_bit_fault = 0;
	aet_ctrl->both_fault = 0;
	aet_ctrl->hash_conflict_num1 = 0;
	aet_ctrl->hash_conflict_num2 = 0;
	aet_ctrl->set_pending_page_num = 0;
	aet_ctrl->sl1mfn_num = 0;
	aet_ctrl->last_set_sl1mfn_num = 0;
	aet_ctrl->last_two_set_sl1mfn_num = 0;
	aet_ctrl->set_sl1mfn_page_num = 0;
	aet_ctrl->set_num = 0;
	aet_ctrl->vmexit_num = 0;
	aet_ctrl->mem_now = 0;
	aet_ctrl->mem_last = 0;
	aet_ctrl->track_fault_time = 0;
	aet_ctrl->track_fault_time_last = 0;
	aet_ctrl->rand_track_time = 0;
	aet_ctrl->rand_track_num = 0;
	aet_ctrl->dtlb_miss_now = 0;
	aet_ctrl->dtlb_miss_last = 0;
	aet_ctrl->hot_set_time = 0;
	aet_ctrl->hot_set_pos = 0;
	aet_ctrl->last_set_num = 0;
	aet_ctrl->add_to_all_sl1mfn_time = 0;
	aet_ctrl->track_aet_time = 0;
	aet_ctrl->aet_func_num = 0;
	aet_ctrl->sleep = 0;
	aet_ctrl->track_fault_time = 0;
	aet_ctrl->add_to_hot_set_time = 0;
	aet_ctrl->add_to_sl1mfn_fail = 0;
	memset(aet_ctrl->all_sl1mfn_hash, 0, sizeof(aet_ctrl->all_sl1mfn_hash));
	memset(aet_ctrl->hot_set, 0, sizeof(aet_ctrl->hot_set));
	//memset(aet_ctrl->valid_node_count, 0, sizeof(aet_ctrl->valid_node_count));
	memset(aet_ctrl->hns_, 0, sizeof(aet_ctrl->hns_));
	memset(aet_ctrl->aet_hist_, 0, sizeof(aet_ctrl->aet_hist_));
	memset(aet_ctrl->longest_aet_hist_pos, 0, sizeof(aet_ctrl->longest_aet_hist_pos));
	memset(aet_ctrl->node_count_, 0, sizeof(aet_ctrl->node_count_));
	memset(aet_ctrl->tot_ref_, 0, sizeof(aet_ctrl->tot_ref_));
	memset(aet_ctrl->lps, 0, sizeof(aet_ctrl->lps));
	memset(aet_ctrl->tvs, 0, sizeof(aet_ctrl->tvs));
}

int main(int argc, char** argv) {
	int ch;
	int do_aet = 0;
	int aet_time = 0;
	int track_rate = 1;
	unsigned long n;
	int hss;
	//printf("----------\nstart:\n");
	while ((ch = getopt(argc, argv, "s:rc:l:t:x:h:")) != -1) {
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
			case 't':
				//printf("option t:%s\n", optarg);
				aet_time = atoi(optarg);
			case 'c':
				//printf("option c:%s\n", optarg);
				do_aet = 1;
				n = strtoul(optarg, NULL, 0);
				break;
			case 'l':
				limit = atoi(optarg);
				//printf("option l limit:%d\n", limit);
				break;
			case 'x':
				track_rate = atoi(optarg);
				set_track_rate(track_rate);
				break;
			case 'h':
				hss = atoi(optarg);
				aet_ctrl->hot_set_size = hss;
				printf("modify hss:%d\n", hss);
				break;
			default:
				break;
		}
	}

	if (do_aet) { 
		aet_process(1, n, aet_time);
	}

	return 0;
}
