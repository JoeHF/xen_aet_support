#ifndef _PUBLIC_AET_H
#define _PUBLIC_AET_H
#define SHARED_DATA_PML4 270
#define LRU_LIST_VIRT_PML4 271
#define SHARED_DATA_START (PML4_ADDR(270))
#define LRU_LIST_VIRT_START     (PML4_ADDR(271))
/* some switchers*/
#define LRU_FLAG 0
#define SAMPLE_FLAG 1

#define TRACK_RATE 16
#define TLB_ENTRIES 512
#define MAX_DOM_NR 2
#define MAX_PAGE_NUM 50000
#define MAX_ARRAY_SIZE 100000
#define MAX_PENDING_PAGE 10000
/*#define HOT_SET_END 100000 */
#define HOT_SET_END 0 
#define DEFAULT_HOT_SET_SIZE 2048 
#define MAX_HOT_SET_SIZE 8192

#define DTLB_ENTRY 64
#define HOT_SET_SIZE 10

#define HASH 49997
#define HASH_CONFLICT_NUM 10 

#define LRU_MAX_PAGE_NUM 520000

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
	NO_OP=0, SET_OPEN, SET_TRACK, SET_HOT_SET_SIZE
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
	INVALID=0, PMU_CMD=1, AET_CMD=2,DEBUGREG_CMD=3,FULL_TRACK_CMD=4
};

struct page_reuse_time {
	unsigned long long addr;
	unsigned long last_mem_counter;
};

enum TYPE {
	SET=0, REVERSED, USER_MODE, TLB_COUNTER, MEM_COUNTER, DIFF
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
	unsigned long dtlb_miss;
	unsigned long pf;
	int track_time;
	int sl1mfn;
	int sl1mfn_pos;
};

struct hot_set_member { 
	int sl1mfn_id;
	int sl1mfn_pos_id;
	unsigned long mfn;
};
struct pending_node {
	unsigned long sl1mfn;
	unsigned long va;
	int track_num;
};

struct lru_node { 
	unsigned long mfn;
	int track_time;
	int sl1mfn;
	int sl1mfn_pos;
	struct lru_node *next;
	struct lru_node *last;
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
	unsigned long tracked_aet_magic_count;
	unsigned long page_fault_count;
	unsigned long total_count;
	struct tracked_va tvs[MAX_ARRAY_SIZE];
	struct last_pte lps[HOT_SET_SIZE];
	unsigned long sl4mfn_;
	enum TRACK track_;
	/* aet related data structure */
	unsigned long aet_hist_[MAX_DOM_NR][MAX_PAGE_NUM];	
	int longest_aet_hist_pos[MAX_DOM_NR];
	unsigned long tot_ref_[MAX_DOM_NR];
	unsigned long cold_miss_[MAX_DOM_NR];
	unsigned long lru_cold_miss_[MAX_DOM_NR];

	unsigned long node_count_[MAX_DOM_NR];
	struct hash_node hns_[MAX_DOM_NR][HASH][HASH_CONFLICT_NUM];
	unsigned long hash_conflict_num1;
	unsigned long hash_conflict_num2;
	/* add to pending set */
	unsigned long set_num;
	int sl1mfn_num;
	int last_sl1mfn_num;
	int sl1mfn_start;
	struct pending_node all_sl1mfn[MAX_PENDING_PAGE];
	unsigned long set_sl1mfn_page_num;
	unsigned long set_pending_page_num;
	/* The following variable is used by iss16*/
	unsigned long vmexit_num;
	unsigned long surplus_total;
	unsigned long surplus_time;
	unsigned long dtlb_miss_now;
	unsigned long dtlb_miss_last;
	unsigned long mem_now;
	unsigned long mem_last;
	unsigned long track_fault_time;
	/* for debug*/
	int last_set_num;
	/* hot set related data structrue */
	int hot_set_end; 
	int hot_set_size;

	int hot_set_time;
	int hot_set_pos;
	struct hot_set_member hot_set[MAX_HOT_SET_SIZE];
	int add_to_hot_set_time;
	/* reset aet hist and hot set*/
	int reset;
	int sleep;
	/* lru related structrue */
	struct lru_node lru_head;
	struct lru_node *lru_head_pointer;
	struct lru_node *lru_list;
	unsigned long lru_node_page_nr;
	unsigned long lru_hist_[LRU_MAX_PAGE_NUM];
	int endless;
	int lru_length;
	int lru_list_pos;
	/* monitor */
	unsigned long add_to_all_sl1mfn_time;
	unsigned long track_aet_time;
	unsigned long aet_func_num;
	unsigned long add_to_hot_set_num;
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
void add_tracked_aet_magic_count(void);
void add_page_fault_count(void);
void add_last_shadow_l1e(unsigned long sl1mfn, unsigned long va);
void get_last_shadow_l1e(unsigned long *sl1mfn, unsigned long *va);

void add_user_mode_fault_count(unsigned long va, unsigned long l1, unsigned long l1p, unsigned long ec, unsigned long mc);
void add_reserved_bit_fault_count(void);
void add_both_fault_count(void);

/* add counter */
void add_tlb_counter(int track_time, unsigned long old_value, unsigned long new_value);
void add_mem_counter(int track_time, unsigned long old_value, unsigned long new_value);
void add_diff(int track_time, unsigned long tlb_diff, unsigned long mem_diff);


/* The following function is used to debug */
int get_track(void);
void set_aet_start(unsigned long sl4mfn);
int get_aet_start(unsigned long *sl4mfn);

/* The following function is used for aet calculation */
int track_aet_fault(int domain_id, unsigned long mfn, unsigned long mem_counter, unsigned long l3, unsigned long dtlb_miss, unsigned long sl1mfn);

/* The following funcion is used for debug register*/
void set_debug_reg(unsigned long va, int cpu_id, void *v);
void hc_set_debug_reg(unsigned long va, int cpu_id);
void track_debug_reg(unsigned long va);

/* The following function is used to add to pending set */
void add_to_all_sl1mfn(unsigned long sl1mfn, unsigned long va);
void rand_track_page(void);

/* The following function is used by iss16*/
void add_vmexit_num(void);
#endif
