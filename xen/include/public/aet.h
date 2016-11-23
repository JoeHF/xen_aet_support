#ifndef _PUBLIC_AET_H
#define _PUBLIC_AET_H

#define SHARED_DATA_PML4 270
#define SHARED_DATA_START (PML4_ADDR(270))

#define CONSECUTIVE_SET_PAGE 16 
#define MAX_DOM_NR 2
#define MAX_PAGE_NUM 1024
#define MAX_ARRAY_SIZE 100000

#define HOT_SET_SIZE 10
enum AET_CMD_OP
{
	NO_OP=0, SET_OPEN, SET_TRACK
};

enum TRACK 
{
	L1_TRACK=0, L4_TRACK, PAGE_FAULT_COUNT
};

enum AET_TRACK_OPEN
{
	CLOSED=0, OPEN
};

enum CMD_OP
{
	INVALID=0, PMU_CMD=1, AET_CMD=2
};

struct page_reuse_time {
	unsigned long long addr;
	unsigned long last_mem_counter;
};

enum TYPE {
	SET=0, REVERSED, USER_MODE
};

struct tracked_va {
	unsigned long va;
	unsigned long l1p;
	unsigned long l1;
	enum TYPE type;
	unsigned long ec;
};

struct last_pte {
	unsigned long va;
	unsigned long sl1mfn;
};

struct AET_ctrl {
	int start_;
	enum AET_TRACK_OPEN open_;
	unsigned long last_pte_start;
	unsigned long last_pte_end;
	unsigned long user_mode_fault;
	unsigned long reserved_bit_fault;
	unsigned long both_fault;
	unsigned long set_aet_magic_count;
	unsigned long reversed_aet_magic_count;
	unsigned long tracked_aet_magic_count1;
	unsigned long tracked_aet_magic_count2;
	unsigned long page_fault_count;
	unsigned long total_count;
	struct tracked_va tvs[MAX_ARRAY_SIZE];
	struct last_pte lps[HOT_SET_SIZE];
	unsigned long sl4mfn_;
	enum TRACK track_;
	struct page_reuse_time prt_[MAX_DOM_NR][MAX_PAGE_NUM];	
};

void aet_init(void);
void set_aet_cmd(enum AET_CMD_OP aet_cmd, unsigned long arg1, unsigned long arg2);
int is_l4_track(void);
int is_l1_track(void);
int is_page_fault_count(void);
int is_aet_track_open(void);
int l1_set_over(int count);
void add_set_aet_magic_count(unsigned long va, unsigned long l1, unsigned long l1p, unsigned long ec);
void add_reversed_aet_magic_count(unsigned long va, unsigned long l1);
void add_tracked_aet_magic_count1(void);
void add_tracked_aet_magic_count2(void);
void add_page_fault_count(void);
void add_last_shadow_l1e(unsigned long sl1mfn, unsigned long va);
void get_last_shadow_l1e(unsigned long *sl1mfn, unsigned long *va);

void add_user_mode_fault_count(unsigned long va, unsigned long l1, unsigned long l1p, unsigned long ec);
void add_reserved_bit_fault_count(void);
void add_both_fault_count(void);

/* The following function is used to debug */
int get_track(void);
void set_aet_start(unsigned long sl4mfn);
int get_aet_start(unsigned long *sl4mfn);
#endif
