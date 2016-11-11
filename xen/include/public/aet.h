#ifndef _PUBLIC_AET_H
#define _PUBLIC_AET_H

#define SHARED_DATA_PML4 270

#define SHARED_DATA_START (PML4_ADDR(270))

#define MAX_DOM_NR 2
#define MAX_PAGE_NUM 1024

struct page_reuse_time {
	unsigned long long addr;
	unsigned long last_mem_counter;
};

struct AET_ctrl {
	struct page_reuse_time page_set[MAX_DOM_NR][MAX_PAGE_NUM];	
};

void aet_init(void);
#endif
