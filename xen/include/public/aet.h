#ifndef _PUBLIC_AET_H
#define _PUBLIC_AET_H
#define SHARED_DATA_PML4 270

#define SHARED_DATA_START (PML4_ADDR(270))

#define MAX_DOM_NR 2
#define MAX_PAGE_NUM 1024

enum AET_CMD_OP
{
	NO_OP=0, OPEN, SET_TRACK
};

enum TRACK 
{
	NONE=0, L1_TRACK, L4_TRACK
};

enum CMD_OP
{
	INVALID=0, PMU_CMD=1, AET_CMD=2
};

struct page_reuse_time {
	unsigned long long addr;
	unsigned long last_mem_counter;
};

struct AET_ctrl {
	int start_;
	unsigned long sl4mfn_;
	enum TRACK track_;
	struct page_reuse_time prt_[MAX_DOM_NR][MAX_PAGE_NUM];	
};

void aet_init(void);
void set_aet_cmd(enum AET_CMD_OP aet_cmd, unsigned long arg1, unsigned long arg2);

/* The following function is used to debug */
int get_track(void);
void set_aet_start(unsigned long sl4mfn);
int get_aet_start(unsigned long *sl4mfn);
#endif
