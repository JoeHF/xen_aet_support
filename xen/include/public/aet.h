#ifndef _PUBLIC_AET_H
#define _PUBLIC_AET_H
#define SHARED_DATA_PML4 270
#define SHARED_DATA_START (PML4_ADDR(270))

#define CONSECUTIVE_SET_PAGE 2 
#define MAX_DOM_NR 2
#define MAX_PAGE_NUM 50000
#define MAX_ARRAY_SIZE 40000

#define HOT_SET_SIZE 10

#define HASH 49997
#define HASH_CONFLICT_NUM 2

#define DEBUG_DR6 0xffff0fff
#define DEBUG_DR7 0xbbbb07ff

#define write_debugreg(reg, val) do {                       \
	unsigned long __val = val;                              \
	asm volatile ( "mov %0,%%db" #reg : : "r" (__val) );    \
} while (0)

#define read_debugreg(reg) ({                               \
    unsigned long __val;                                    \
    asm volatile ( "mov %%db" #reg ",%0" : "=r" (__val) );  \
    __val;                                                  \
})

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
	INVALID=0, PMU_CMD=1, AET_CMD=2,DEBUGREG_CMD=3
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
	unsigned long mc;
};

struct last_pte {
	unsigned long va;
	unsigned long sl1mfn;
};

struct hash_node {
	unsigned long mfn;
	unsigned long mem_counter;
	unsigned long pf;
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
	unsigned long aet_hist_[MAX_DOM_NR][MAX_PAGE_NUM];	
	unsigned long tot_ref_[MAX_DOM_NR];
	unsigned long node_count_[MAX_DOM_NR];
	struct hash_node hns_[MAX_DOM_NR][HASH][HASH_CONFLICT_NUM];
	unsigned long hash_conflict_num;
};

void aet_init(void);
void set_aet_cmd(enum AET_CMD_OP aet_cmd, unsigned long arg1, unsigned long arg2);
int is_l4_track(void);
int is_l1_track(void);
int is_page_fault_count(void);
int is_aet_track_open(void);
int l1_set_over(int count);
void add_set_aet_magic_count(unsigned long va, unsigned long l1, unsigned long l1p, unsigned long ec, unsigned long mc);
void add_reversed_aet_magic_count(unsigned long va, unsigned long l1);
void add_tracked_aet_magic_count1(void);
void add_tracked_aet_magic_count2(void);
void add_page_fault_count(void);
void add_last_shadow_l1e(unsigned long sl1mfn, unsigned long va);
void get_last_shadow_l1e(unsigned long *sl1mfn, unsigned long *va);

void add_user_mode_fault_count(unsigned long va, unsigned long l1, unsigned long l1p, unsigned long ec, unsigned long mc);
void add_reserved_bit_fault_count(void);
void add_both_fault_count(void);

/* The following function is used to debug */
int get_track(void);
void set_aet_start(unsigned long sl4mfn);
int get_aet_start(unsigned long *sl4mfn);

/* The following function is used for aet calculation */
void track_aet_fault(int domain_id, unsigned long mfn, unsigned long mem_counter);

/* The folloing funciont is used for debug register*/
void set_debug_reg(unsigned long va, int cpu_id, void *v);
void hc_set_debug_reg(unsigned long va, int cpu_id);
void track_debug_reg(unsigned long va);


#endif
