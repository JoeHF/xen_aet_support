#include <public/aet.h>
#include <xen/config.h>
#include <xen/types.h>
#include <xen/lib.h>
#include <xen/string.h>
#include <xen/guest_access.h>
#include <xen/lib.h>
#include <asm/page.h>
#include <asm/bitops.h>
#include <asm/apic.h>
#include <xen/kernel.h>
#include <public/sched.h>
#include <public/xc_reserved_op.h>

static struct AET_ctrl *aet_ctrl;
static unsigned int randseed;
static char* AET_CMD_NAME[4] = {"NO_OP", "SET_OPEN", "SET_TRACK", "SET_HOT_SIZE"};
static char* TRACK_NAME[3] = {"L1_TRACK", "L4_TRACK", "PAGE_FAULT_COUNT"};
static char* OPEN_NAME[2] = {"CLOSED", "OPEN"};

static int add_to_hist(int pos); 
static void add_to_lru_list_head(struct lru_node* ln); 
static struct lru_node* find_lru_node(unsigned long mfn, int *pos); 
static struct lru_node* add_lru_node(unsigned long mfn); 
void set_aet_start(unsigned long sl4mfn) {
	aet_ctrl->start_ = 1;
	aet_ctrl->sl4mfn_ = sl4mfn;
}

int get_aet_start(unsigned long *sl4mfn) {
	*sl4mfn = aet_ctrl->sl4mfn_;
	return aet_ctrl->start_;
}

static void set_aet_track_open(enum AET_TRACK_OPEN open, enum TRACK track) {
	if (open)
		aet_ctrl->track_ = track;
	else
		aet_ctrl->track_ = CLOSED;
	aet_ctrl->open_ = open;
}

static void set_track(enum TRACK track) {
	if (!is_aet_track_open()) {
		printk("aet set track fail! aet track not open\n");
		return;
	}

	aet_ctrl->track_ = track;
}

int get_track(void) {
	return aet_ctrl->track_;
}

int is_l4_track(void) {
	return is_aet_track_open() && (aet_ctrl->track_ == L4_TRACK);
}

int is_l1_track(void) {
	return is_aet_track_open() && (aet_ctrl->track_ == L1_TRACK);
}

int is_page_fault_count(void) {
	return is_aet_track_open() && (aet_ctrl->track_ == PAGE_FAULT_COUNT);
}

int is_aet_track_open(void) {
	return aet_ctrl->open_ == OPEN;
}

void add_set_aet_magic_count(unsigned long va, unsigned long l1, unsigned long l1p, unsigned long ec, unsigned long mc) {
	if (aet_ctrl->total_count + 10 < MAX_ARRAY_SIZE) {
		aet_ctrl->tvs[aet_ctrl->total_count].va = va;
		aet_ctrl->tvs[aet_ctrl->total_count].type = SET;
		aet_ctrl->tvs[aet_ctrl->total_count].l1 = l1;
		aet_ctrl->tvs[aet_ctrl->total_count].l1p = aet_ctrl->vmexit_num;
		aet_ctrl->tvs[aet_ctrl->total_count].ec = ec;
		aet_ctrl->tvs[aet_ctrl->total_count].mc = mc;
		aet_ctrl->total_count++;
	}

	aet_ctrl->set_aet_magic_count++;
}

void add_reversed_aet_magic_count(unsigned long va, unsigned long l1) {
	//printk("%s va:%lx l1:%lx vmexit:%lu\n", __func__, va, l1, aet_ctrl->vmexit_num);
	if (aet_ctrl->total_count + 10 < MAX_ARRAY_SIZE) {
//		aet_ctrl->tvs[aet_ctrl->total_count].va = va;
//		aet_ctrl->tvs[aet_ctrl->total_count].type = REVERSED;
//		aet_ctrl->tvs[aet_ctrl->total_count].l1 = l1;
//		aet_ctrl->total_count++;
	}

	aet_ctrl->reversed_aet_magic_count++;
}

void add_tracked_aet_magic_count(void) {
	aet_ctrl->tracked_aet_magic_count++;
}

void add_page_fault_count(void) {
	aet_ctrl->page_fault_count++;
}

void add_last_shadow_l1e(unsigned long sl1mfn, unsigned long va) {
	int pos = aet_ctrl->last_pte_end;
	while (pos != aet_ctrl->last_pte_start) {
		if (aet_ctrl->lps[pos].va == va) {
			return;
		}
		else {
			pos = (pos + 1) % HOT_SET_SIZE;
		}
	}

	pos = aet_ctrl->last_pte_start;
	aet_ctrl->last_pte_start = (aet_ctrl->last_pte_start + 1) % HOT_SET_SIZE;
	aet_ctrl->lps[pos].sl1mfn = sl1mfn;
	aet_ctrl->lps[pos].va = va;
}

void get_last_shadow_l1e(unsigned long *sl1mfn, unsigned long *va) {
	int pos = aet_ctrl->last_pte_end;
	if ((aet_ctrl->last_pte_start + 1) % HOT_SET_SIZE == aet_ctrl->last_pte_end) {
		*va = aet_ctrl->lps[pos].va;
		*sl1mfn = aet_ctrl->lps[pos].sl1mfn;
		aet_ctrl->last_pte_end = (aet_ctrl->last_pte_end + 1) % HOT_SET_SIZE;
	}
	else {
		*va = 0;
		*sl1mfn = 0;
	}
}

void add_user_mode_fault_count(unsigned long va, unsigned long l1, unsigned long l1p, unsigned long ec, unsigned long mc) {
	aet_ctrl->user_mode_fault++;
	if (aet_ctrl->total_count + 10 < MAX_ARRAY_SIZE) {
		aet_ctrl->tvs[aet_ctrl->total_count].va = va;
		aet_ctrl->tvs[aet_ctrl->total_count].type = USER_MODE;
		aet_ctrl->tvs[aet_ctrl->total_count].l1 = l1;
		aet_ctrl->tvs[aet_ctrl->total_count].l1p = aet_ctrl->vmexit_num;
		aet_ctrl->tvs[aet_ctrl->total_count].ec = ec;
		aet_ctrl->tvs[aet_ctrl->total_count].mc = mc;
		
		aet_ctrl->total_count++;
	}

}

static void add_counter(int track_time, enum TYPE type, unsigned long old_value, unsigned long new_value) { 
	if (aet_ctrl->total_count + 10 < MAX_ARRAY_SIZE) {
		aet_ctrl->tvs[aet_ctrl->total_count].va = track_time;
		aet_ctrl->tvs[aet_ctrl->total_count].type = type;
		aet_ctrl->tvs[aet_ctrl->total_count].l1 = 0;
		aet_ctrl->tvs[aet_ctrl->total_count].l1p = aet_ctrl->vmexit_num;
		aet_ctrl->tvs[aet_ctrl->total_count].ec = old_value;
		aet_ctrl->tvs[aet_ctrl->total_count].mc = new_value;
		
		aet_ctrl->total_count++;
	}

}

void add_tlb_counter(int track_time, unsigned long old_value, unsigned long new_value) { 
	add_counter(track_time, TLB_COUNTER, old_value, new_value);
}

void add_mem_counter(int track_time, unsigned long old_value, unsigned long new_value) { 
	add_counter(track_time, MEM_COUNTER, old_value, new_value);
}

void add_diff(int track_time, unsigned long tlb_diff, unsigned long mem_diff) { 
	add_counter(track_time, DIFF, tlb_diff, mem_diff);
}

void add_reserved_bit_fault_count(void) {
	aet_ctrl->reserved_bit_fault++;
}

void add_both_fault_count(void) {
	aet_ctrl->both_fault++;
}

/* The following function is used for aet calculation */
const unsigned long domain = 256;
static unsigned long domain_value_to_index(unsigned long value)
{
    unsigned long loc = 0, step = 1;
    unsigned long index = 0;
    while (loc + step * domain < value) {
        loc += step * domain;
        step *= 2;
        index += domain;
    }

    while (loc < value) {
        index++;
        loc += step;
    }
    return index;
}
/*
static void add_to_aet_histogram(int domain_id, unsigned long old_mc, unsigned long new_mc) {
	unsigned long compressed_dis;
//	printk("%s\n", __func__);
	if (old_mc > new_mc) {
		printk("[WARNING] old mem counter is larger than the new mem counter\n");
		return;
	}

	compressed_dis = domain_value_to_index(new_mc - old_mc);	
	//printk("real dis:%lu compressed_dis:%lu\n", new_mc - old_mc, compressed_dis);
	if (compressed_dis >= MAX_PAGE_NUM) {
		printk("[WARNING] compressed dis is larger than array size new_mc:%lu old_mc:%lu compressed_dis:%lu\n", new_mc, old_mc, compressed_dis);
		return;
	}

	//printk("cd:%lu longest:%d\n", compressed_dis, aet_ctrl->longest_aet_hist_pos[domain_id - 1]);
	if (compressed_dis > aet_ctrl->longest_aet_hist_pos[domain_id - 1]) { 
		aet_ctrl->longest_aet_hist_pos[domain_id - 1] = compressed_dis;
	}

	aet_ctrl->aet_hist_[domain_id - 1][compressed_dis]++;
	aet_ctrl->tot_ref_[domain_id - 1]++;
}
*/

static void add_to_aet_histogram_pf(int domain_id, unsigned long old_pf, unsigned long new_pf) {
	unsigned long compressed_dis;
	if (new_pf <= old_pf) { 
		printk("new pf:%lu less than old pf:%lu\n", old_pf, new_pf);
		return;
	}

	if (new_pf - old_pf < aet_ctrl->hot_set_size)
		printk("[WARNING] the distance is less than hot set size:%d\n", aet_ctrl->hot_set_size);

	if (SAMPLE_FLAG) { 
		compressed_dis = domain_value_to_index((new_pf - old_pf) * TRACK_RATE);
	}
	else { 
		compressed_dis = domain_value_to_index(new_pf - old_pf);	
	}

	//compressed_dis = domain_value_to_index(new_pf - old_pf - aet_ctrl->hot_set_size);	
	add_user_mode_fault_count(11, 0, 0, new_pf - old_pf, compressed_dis);
	if (compressed_dis >= MAX_PAGE_NUM) {
		printk("[WARNING] compressed dis is larger than array size new_mc:%lu old_mc:%lu compressed_dis:%lu\n", new_pf, old_pf, compressed_dis);
		return;
	}

	//printk("cd:%lu longest:%d\n", compressed_dis, aet_ctrl->longest_aet_hist_pos[domain_id - 1]);
	if (compressed_dis > aet_ctrl->longest_aet_hist_pos[domain_id]) { 
		aet_ctrl->longest_aet_hist_pos[domain_id] = compressed_dis;
	}

	aet_ctrl->aet_hist_[domain_id][compressed_dis]++;
	aet_ctrl->tot_ref_[domain_id]++;
}

/*
 * instead of estimating aet first hist value, do that when doing aet
 */
/*
static void add_to_aet_first_hist2(int domain_id,
								   unsigned long old_dtlb,
								   unsigned long new_dtlb,
								   unsigned long old_mem,
								   unsigned long new_mem) { 
	unsigned long dtlb_miss_diff;
	unsigned long mem_diff;
	if (old_dtlb >= new_dtlb) { 
		printk("[WARNING] old dtlb counter:%lu is larger than the new dtlb counter:%lu\n", old_dtlb, new_dtlb);
		return;
	}	

	if (old_mem > new_mem) {
		printk("[WARNING] old mem counter:%lu is larger than the new mem counter:%lu\n", old_mem, new_mem);
		return;
	}

	dtlb_miss_diff = new_dtlb - old_dtlb;
	mem_diff = new_mem - old_mem;
	if (mem_diff <= dtlb_miss_diff) { 
		printk("[WARNING] mem_diff:%lu is less than dtlb_miss_diff:%lu\n", mem_diff, dtlb_miss_diff);
		return;
	}

	aet_ctrl->surplus_total += mem_diff - dtlb_miss_diff;
	aet_ctrl->surplus_time++;
}
*/

/*
static void add_to_aet_first_hist(int domain_id, 
		unsigned long old_l3, unsigned long new_l3,
		unsigned long old_mem, unsigned long new_mem) {
	unsigned long l3_diff;
	unsigned long mem_diff;
	unsigned long surplus;
	unsigned long total_page;
	if (old_l3 > new_l3) {
		printk("[WARNING] old l3 counter is larger than the new l3 counter");
		return;
	}
	
	if (old_mem > new_mem) {
		printk("[WARNING] old mem counter is larger than the new mem counter");
		return;
	}

	l3_diff = new_l3 - old_l3;
	mem_diff = new_mem - old_mem;
	if (l3_diff >= mem_diff) {
		printk("[WARNING] mem_diff less than l3_diff l3diff:%lu memdiff:%lu", l3_diff, mem_diff);
		return;
	}


    surplus = (mem_diff - l3_diff);
	total_page = aet_ctrl->node_count_[domain_id - 1] * TRACK_RATE;
	surplus = surplus / total_page;

	add_user_mode_fault_count(0, surplus, 0, l3_diff, mem_diff); // for debug	
	aet_ctrl->aet_hist_[domain_id - 1][1] += surplus;
	aet_ctrl->tot_ref_[domain_id - 1] += surplus;
}
*/

/* steal from shadow code */
typedef l1_pgentry_t shadow_l1e_t;
#define SH_type_l1_64_shadow   (8U) /* shadowing a 64-bit L1 page */
#define SH_type_fl1_64_shadow  (9U) /* L1 shadow for 64-bit 2M superpg */
#define SH_type_l1_shadow  SH_type_l1_64_shadow
#define SH_type_fl1_shadow SH_type_fl1_64_shadow
#define SH_L1E_AET_MAGIC 0x7ff8000000000400ULL
static inline u32 shadow_l1e_get_flags(shadow_l1e_t sl1e)
{ return l1e_get_flags(sl1e); }

static void track_specific_pte(int sl1mfn_id, int sl1mfn_pos_id) { 
	unsigned long sl1mfn = aet_ctrl->all_sl1mfn[sl1mfn_id].sl1mfn;
	shadow_l1e_t *sp, *sl1e;
	unsigned long user_bit = 0x4;
	unsigned long access_bit = 0x20;


	if (mfn_to_page(sl1mfn)->u.sh.type == SH_type_l1_shadow
		|| mfn_to_page(sl1mfn)->u.sh.type == SH_type_fl1_shadow) {
		sp = map_domain_page(sl1mfn);
		sl1e = sp + sl1mfn_pos_id;
		if ((shadow_l1e_get_flags(*sl1e) & _PAGE_USER)
			&& (shadow_l1e_get_flags(*sl1e) & _PAGE_RW)
			&& (shadow_l1e_get_flags(*sl1e) & _PAGE_PRESENT)
			&& (sl1e->l1 & SH_L1E_AET_MAGIC) == 0) { 
			sl1e->l1 |= SH_L1E_AET_MAGIC;
			sl1e->l1 &= (~user_bit);
			sl1e->l1 &= (~access_bit);
		}
		
	}
}

static void lru_func(unsigned long mfn, int add_to_hist_) { 
	int pos;
	struct lru_node* new_node = find_lru_node(mfn, &pos);
//	printk("%s begin\n", __func__);
	if (new_node == NULL) {
		new_node = add_lru_node(mfn);
		add_user_mode_fault_count(13, 0, 0, mfn, 0);
		aet_ctrl->lru_cold_miss_[current->domain->domain_id]++;
		if (new_node == NULL) {
			printk("[ERROR] add lru node fail\n");
		}
	}
	else {
		add_user_mode_fault_count(14, 0, 0, mfn, pos);
		if (add_to_hist_) { 
			//printk("%s add to hist pos:%d\n", __func__, pos);
			add_to_hist(pos);
			//add_to_hist(pos - aet_ctrl->hot_set_size);
		}
	}

	if (new_node != NULL)
		add_to_lru_list_head(new_node);
}

static unsigned long add_to_hot_set(struct hot_set_member *hsm) { 
	struct hot_set_member *current_hsm = aet_ctrl->hot_set + aet_ctrl->hot_set_pos;
	unsigned long pop_mfn = 0;
	aet_ctrl->add_to_hot_set_num++;
//	printk("%s begin\n", __func__);
	if (aet_ctrl->hot_set_end != 0 && aet_ctrl->hot_set_time >= aet_ctrl->hot_set_end) { 
		if (aet_ctrl->hot_set_time == aet_ctrl->hot_set_end) { 
			printk("hot set fill time is up to limit\n");
			aet_ctrl->hot_set_time++;
		}
		return pop_mfn;
	}

	if (aet_ctrl->sleep == 1)
		return pop_mfn;

	aet_ctrl->add_to_hot_set_time++;
	if (current_hsm->mfn != 0) { 
		/* for lru compare */
		track_specific_pte(current_hsm->sl1mfn_id, current_hsm->sl1mfn_pos_id);
		add_set_aet_magic_count(10, 0, 0, current_hsm->mfn, current_hsm->mfn);
		pop_mfn = current_hsm->mfn;
		/* pop from hot set and put lru node at the head */
		//lru_func(current_hsm->mfn, 0);
	}

	current_hsm->mfn = hsm->mfn;
	current_hsm->sl1mfn_id = hsm->sl1mfn_id;
	current_hsm->sl1mfn_pos_id = hsm->sl1mfn_pos_id;
	if (aet_ctrl->hot_set_size == 0) { 
		aet_ctrl->hot_set_size = DEFAULT_HOT_SET_SIZE;
		printk("hot set size if zero!!!!!!!!!! set to default:%d\n", aet_ctrl->hot_set_size);
	}

	aet_ctrl->hot_set_pos = (aet_ctrl->hot_set_pos + 1) % aet_ctrl->hot_set_size; 
	if (aet_ctrl->hot_set_pos == 0) { 
		aet_ctrl->hot_set_time++;
	}

	return pop_mfn;
}

static struct hash_node* find_in_hash_set(int domain_id, unsigned long mfn, struct hot_set_member *hsm) { 
	int key;
	int hash_pos;
	struct hash_node *hn;
//	printk("%s begin\n", __func__);
	key = mfn % HASH;
	for (hash_pos = 0 ; hash_pos < HASH_CONFLICT_NUM ; hash_pos++) {
		hn = aet_ctrl->hns_[domain_id][key] + hash_pos;
		if (hn->mfn == mfn) { 
			hsm->sl1mfn_id = hn->sl1mfn;
			hsm->sl1mfn_pos_id = hn->sl1mfn_pos;
			hsm->mfn = mfn;
			return hn;
		}
	}

	hn = NULL;
	hsm = NULL;
	//printk("[WARN] track aet fault can not find hash conflict because it is already check in rand track page\n");
	aet_ctrl->hash_conflict_num2++;
	return hn;
}
static void aet_func(int domain_id, struct hash_node *hn) { 
//	printk("%s begin\n", __func__);
    aet_ctrl->aet_func_num++;
	if (hn->track_time > 0) { 
		add_to_aet_histogram_pf(domain_id, hn->pf, aet_ctrl->track_fault_time);
		add_user_mode_fault_count(15, hn->mfn, 0, hn->pf, aet_ctrl->track_fault_time);	
	}
	else { 
		aet_ctrl->cold_miss_[domain_id]++;
	}

	hn->pf = aet_ctrl->track_fault_time;
	//hn->mem_counter = mem_counter;
	//hn->dtlb_miss = dtlb_miss;
	hn->track_time++;
}

int track_aet_fault(int domain_id, 
					 unsigned long mfn, 
					 unsigned long mem_counter, 
					 unsigned long dtlb_load_miss,
					 unsigned long dtlb_store_miss,
					 unsigned long sl1mfn) {
	unsigned long dtlb_miss;
	struct hash_node *hn;
	struct hot_set_member hsm;
	unsigned long pop_mfn;
	unsigned long start = get_localtime_us(current->domain);
	unsigned long end;
//	printk("%s begin\n", __func__);
	dtlb_miss = dtlb_load_miss + dtlb_store_miss;
	aet_ctrl->mem_now = mem_counter;
	aet_ctrl->dtlb_miss_now = dtlb_miss;
	aet_ctrl->track_fault_time++;

	if (aet_ctrl->sleep == 1)
		goto out;
	/* find in hash set*/
	hn = find_in_hash_set(domain_id, mfn, &hsm);
	
	if (hn != NULL && hsm.mfn != 0) { 
		/*
		 * add to hot set
		 * if hot set is full
		 * pop one from top and track that page
		 */
		pop_mfn = add_to_hot_set(&hsm);
		/* aet function*/
		aet_func(domain_id, hn);	
		/* lru function*/
		//printk("[DEBUG] hot set time:%d tot ref:%lu\n", aet_ctrl->hot_set_time, aet_ctrl->tot_ref_[0]);
		if (LRU_FLAG)
			lru_func(hn->mfn, 1);
	}
	else { 
		if (!SAMPLE_FLAG)
			printk("[WARN] can not find in hash set\n");
	}

out:	
	end = get_localtime_us(current->domain);
	aet_ctrl->track_aet_time += (end - start);
	return 0;
}

/* The folloing funciont is used for debug register */
typedef struct {
	unsigned long dr0;
	unsigned long dr6;
	unsigned long dr7;
	struct vcpu *v;
} dr_on_cpu;

void __set_debug_reg(void *info) {
	unsigned long dr0, dr6, dr7;
	unsigned int cpu_id;
	struct vcpu *v;
	dr_on_cpu *doc = (dr_on_cpu *)info;
	cpu_id = smp_processor_id();
	dr0 = doc->dr0;
	dr6 = doc->dr6;
	dr7 = doc->dr7;
	v = doc->v;
	//write_debugreg(0, dr0);
	write_debugreg(6, dr6);
	//write_debugreg(7, dr7);

	hvm_funcs.set_debugreg_for_s(v, dr0);
	printk("[joe]%s cpu_id:%u, dr0:%lx, dr6:%lx, dr7:%lx\n", __func__, cpu_id, dr0, dr6, dr7);
}

void set_debug_reg(unsigned long va, int cpu_id, void *v) {
	dr_on_cpu doc;
	cpumask_t c;
	cpumask_clear(&c);
	cpumask_set_cpu(cpu_id, &c);

	cpu_id = smp_processor_id();
	doc.dr0 = va;
	doc.dr6 = DEBUG_DR6;
	doc.dr7 = DEBUG_DR7;
	doc.v = (struct vcpu *)v;

	printk("[joe]%s va:%lx cpu_id:%u\n", __func__, va, cpu_id);
	on_selected_cpus(&c, __set_debug_reg, (void *)(&doc), -1);
}

void __hc_set_debug_reg(void *info) {
	unsigned long dr0, dr6, dr7;
	unsigned int cpu_id;
	dr_on_cpu *doc = (dr_on_cpu *)info;
	cpu_id = smp_processor_id();
	dr0 = doc->dr0;
	dr6 = doc->dr6;
	dr7 = doc->dr7;
	write_debugreg(0, dr0);
	write_debugreg(6, dr6);
	write_debugreg(7, dr7);

	printk("[joe]%s cpu_id:%u, dr0:%lx, dr6:%lx, dr7:%lx\n", __func__, cpu_id, dr0, dr6, dr7);
}

void hc_set_debug_reg(unsigned long va, int cpu_id) {
	dr_on_cpu doc;
	cpumask_t c;
	cpumask_clear(&c);
	cpumask_set_cpu(cpu_id, &c);

	cpu_id = smp_processor_id();
	doc.dr0 = va;
	doc.dr6 = DEBUG_DR6;
	doc.dr7 = DEBUG_DR7;

	printk("[joe]%s va:%lx cpu_id:%u\n", __func__, va, cpu_id);
	on_selected_cpus(&c, __hc_set_debug_reg, (void *)(&doc), -1);
}
void track_debug_reg(unsigned long va) {
}

/* The following function is used to add to pending set */
#define L1_MASK  ((1UL << L2_PAGETABLE_SHIFT) - 1)

/* need to be optimized */
void add_to_all_sl1mfn(unsigned long sl1mfn, unsigned long va) {
	unsigned long va_start = va - (va & L1_MASK & PAGE_MASK);
	int i;
	unsigned long start = get_localtime_us(current->domain);
	unsigned long end;
	//printk("%s sl1mfn:%lx, va:%lx\n", __func__, sl1mfn, va);
	// remove duplicate one
	for (i = aet_ctrl->sl1mfn_num - 1 ; i >= 0 ; i--) {
		if (aet_ctrl->all_sl1mfn[i].sl1mfn == sl1mfn)
			goto out;
	}

	if (aet_ctrl->sl1mfn_num >= MAX_PENDING_PAGE) {
		printk("[joe]%s sl1mfn num exceed MAX_PENDING_PAGE_SIZE\n", __func__);
		goto out;
	}

	aet_ctrl->all_sl1mfn[aet_ctrl->sl1mfn_num].sl1mfn = sl1mfn;
	aet_ctrl->all_sl1mfn[aet_ctrl->sl1mfn_num].va = va_start;
	aet_ctrl->sl1mfn_num++;
	
out:
	end = get_localtime_us(current->domain);
	aet_ctrl->add_to_all_sl1mfn_time += (end - start);
	return;
}

static unsigned int curl_rand(void) { 
	unsigned int r;
	r = randseed = randseed * 1103515245 + 12345;
	return (r << 16) | ((r >> 16) & 0xFFFF);
}

static int rand_track_algorithm(int *hash_conflict) { 
	int i, j, hash_pos;
	unsigned long sl1mfn, va;
	shadow_l1e_t *sp, *sl1e;
	unsigned long mfn;
	int is_hash_conflict;
	int invalid_sl1mfn = 0;
	int already_set = 0;
	int key;
	int count = 0;
	struct hash_node *hn;
	unsigned long user_bit = 0x4;
	unsigned long access_bit = 0x20;
	// for rand track
	int rand_pos = curl_rand() % (TRACK_RATE * 2);
	int counter = 0;
	int rand_skip = 0;
	int rand_chosen = 0;
	int dom = current->domain->domain_id;
	printk("domid:%d\n", dom);
	dom = 1;

	memset(aet_ctrl->hns_, 0, sizeof(aet_ctrl->hns_));
	for (i = 0 ; i < aet_ctrl->sl1mfn_num ; i++) { 
		sl1mfn = aet_ctrl->all_sl1mfn[i].sl1mfn;
		va = aet_ctrl->all_sl1mfn[i].va;
		if (mfn_to_page(sl1mfn)->u.sh.type == SH_type_l1_shadow
			|| mfn_to_page(sl1mfn)->u.sh.type == SH_type_fl1_shadow) {
			sp = map_domain_page(sl1mfn);
			for (j = 0 ; j < 512 ; j++) { 
				sl1e = sp + j;
				if ((shadow_l1e_get_flags(*sl1e) & _PAGE_USER)
					&& (shadow_l1e_get_flags(*sl1e) & _PAGE_RW)
					&& (shadow_l1e_get_flags(*sl1e) & _PAGE_PRESENT)
					&& (sl1e->l1 & SH_L1E_AET_MAGIC) == 0) { 
					mfn = (sl1e->l1 & (PADDR_MASK&PAGE_MASK)) >> PAGE_SHIFT;
					key = mfn % HASH;
					is_hash_conflict = 1;
					for (hash_pos = 0 ; hash_pos < HASH_CONFLICT_NUM ; hash_pos++) {
						hn = aet_ctrl->hns_[dom][key] + hash_pos;
						if (hn->mfn == 0) { 
							hn->mfn = mfn;
							hn->sl1mfn = i;
							hn->sl1mfn_pos = j;
							hn->track_time = 0;
							is_hash_conflict = 0;
							break;
						}
						else if (hn->mfn == mfn) { 
							hn->sl1mfn = i;
							hn->sl1mfn_pos = j;
							hn->track_time = 0;
							is_hash_conflict = 0;
							break;
						}
					}

					if (is_hash_conflict == 0) { 
						count++;
						add_set_aet_magic_count(12, 0, 0, hn->mfn, hn->track_time);
						if (counter == rand_pos) {
							rand_chosen++;
							counter = 0;
							rand_pos = curl_rand() % (TRACK_RATE * 2);
							sl1e->l1 |= SH_L1E_AET_MAGIC;
							sl1e->l1 &= (~user_bit);
							sl1e->l1 &= (~access_bit);
						}
						else { 
							rand_skip++;
							counter++;
						}
					}
					else { 
						(*hash_conflict)++;
					}
				}
				else { 
					if ((sl1e->l1 & SH_L1E_AET_MAGIC) != 0) {
						already_set++;
						mfn = ((sl1e->l1 ^ 0x8000000000000ULL) & (PADDR_MASK&PAGE_MASK)) >> PAGE_SHIFT;
						//printk("sl1e->l1:%lx mfn:%lx\n", sl1e->l1, mfn);
						key = mfn % HASH;
						is_hash_conflict = 1;
						for (hash_pos = 0 ; hash_pos < HASH_CONFLICT_NUM ; hash_pos++) {
							hn = aet_ctrl->hns_[dom][key] + hash_pos;
							if (hn->mfn == 0) { 
								hn->mfn = mfn;
								hn->sl1mfn = i;
								hn->sl1mfn_pos = j;
								hn->track_time = 0;
								is_hash_conflict = 0;
								break;
							}
							else if (hn->mfn == mfn) { 
								hn->sl1mfn = i;
								hn->sl1mfn_pos = j;
								//hn->track_time = 0;
								is_hash_conflict = 0;
								break;
							}
						}
					}
				}
			}
		}
		else { 
			invalid_sl1mfn++;
		}
	}

	aet_ctrl->last_set_num = count;
	if (count == DEFAULT_HOT_SET_SIZE) { 
		aet_ctrl->reset = 3;
		printk("set reset 3\n");
	}
	flush_tlb_local();
	printk("%s sl1mfn_num:%d invalid sl1mfn:%d already_set:%d\n", __func__, aet_ctrl->sl1mfn_num, invalid_sl1mfn, already_set);
	printk("%s count:%d hash_conflict:%d rand_skip:%d/rand_chosen:%d\n", __func__, count, *hash_conflict, rand_skip, rand_chosen);
	return count;
}

static int full_track_algorithm(int *hash_conflict) { 
	int i, j, hash_pos;
	unsigned long sl1mfn, va;
	shadow_l1e_t *sp, *sl1e;
	unsigned long mfn;
	int is_hash_conflict;
	int invalid_sl1mfn = 0;
	int already_set = 0;
	int key;
	int count = 0;
	struct hash_node *hn;
	unsigned long user_bit = 0x4;
	unsigned long access_bit = 0x20;
	int dom = current->domain->domain_id;
	printk("domid:%d last_sl1mfn_num:%d\n", dom, aet_ctrl->last_sl1mfn_num);
	dom = 1;
	//memset(aet_ctrl->hns_, 0, sizeof(aet_ctrl->hns_));
	//for (i = aet_ctrl->last_sl1mfn_num ; i < aet_ctrl->sl1mfn_num ; i++) { 
	for (i = 0 ; i < aet_ctrl->sl1mfn_num ; i++) { 
		sl1mfn = aet_ctrl->all_sl1mfn[i].sl1mfn;
		va = aet_ctrl->all_sl1mfn[i].va;
		if (mfn_to_page(sl1mfn)->u.sh.type == SH_type_l1_shadow
			|| mfn_to_page(sl1mfn)->u.sh.type == SH_type_fl1_shadow) {
			sp = map_domain_page(sl1mfn);
			for (j = 0 ; j < 512 ; j++) { 
				sl1e = sp + j;
				if ((shadow_l1e_get_flags(*sl1e) & _PAGE_USER)
					&& (shadow_l1e_get_flags(*sl1e) & _PAGE_RW)
					&& (shadow_l1e_get_flags(*sl1e) & _PAGE_PRESENT)
					&& (sl1e->l1 & SH_L1E_AET_MAGIC) == 0) { 
					mfn = (sl1e->l1 & (PADDR_MASK&PAGE_MASK)) >> PAGE_SHIFT;
					key = mfn % HASH;
					is_hash_conflict = 1;
					for (hash_pos = 0 ; hash_pos < HASH_CONFLICT_NUM ; hash_pos++) {
						hn = aet_ctrl->hns_[dom][key] + hash_pos;
						if (hn->mfn == 0) { 
							hn->mfn = mfn;
							hn->sl1mfn = i;
							hn->sl1mfn_pos = j;
							hn->track_time = 0;
							is_hash_conflict = 0;
							break;
						}
						else if (hn->mfn == mfn) { 
							hn->sl1mfn = i;
							hn->sl1mfn_pos = j;
							hn->track_time = 0;
							is_hash_conflict = 0;
							break;
						}
					}

					if (is_hash_conflict == 0) { 
						count++;
						add_set_aet_magic_count(12, 0, 0, hn->mfn, hn->track_time);
						sl1e->l1 |= SH_L1E_AET_MAGIC;
						sl1e->l1 &= (~user_bit);
						sl1e->l1 &= (~access_bit);
					}
					else { 
						(*hash_conflict)++;
					}
				}
				else { 
					if ((sl1e->l1 & SH_L1E_AET_MAGIC) != 0) {
						already_set++;
						mfn = ((sl1e->l1 ^ 0x8000000000000ULL) & (PADDR_MASK&PAGE_MASK)) >> PAGE_SHIFT;
						//printk("sl1e->l1:%lx mfn:%lx\n", sl1e->l1, mfn);
						key = mfn % HASH;
						is_hash_conflict = 1;
						for (hash_pos = 0 ; hash_pos < HASH_CONFLICT_NUM ; hash_pos++) {
							hn = aet_ctrl->hns_[dom][key] + hash_pos;
							if (hn->mfn == 0) { 
								hn->mfn = mfn;
								hn->sl1mfn = i;
								hn->sl1mfn_pos = j;
								hn->track_time = 0;
								is_hash_conflict = 0;
								//printk("tracked not in hash\n");
								break;
							}
							else if (hn->mfn == mfn) { 
								hn->sl1mfn = i;
								hn->sl1mfn_pos = j;
								//hn->track_time = 0;
								is_hash_conflict = 0;
								break;
							}
						}

						if (is_hash_conflict) { 
							(*hash_conflict)++;
						}
					}
				}
			}
		}
		else { 
			invalid_sl1mfn++;
		}
	}

	aet_ctrl->last_set_num = count;
	aet_ctrl->last_sl1mfn_num = aet_ctrl->sl1mfn_num + 1;
	if (count == DEFAULT_HOT_SET_SIZE) { 
		aet_ctrl->reset = 3;
		printk("set reset 3\n");
	}
	flush_tlb_local();
	printk("%s sl1mfn_num:%d invalid sl1mfn:%d already_set:%d\n", __func__, aet_ctrl->sl1mfn_num, invalid_sl1mfn, already_set);
	printk("%s count:%d hash_conflict:%d\n", __func__, count, *hash_conflict
);
	return count;
}

void rand_track_page() { 
	int count = 0;
	int hash_conflict = 0;
	int hot_set_time = aet_ctrl->hot_set_time;

	if (aet_ctrl->sl1mfn_num < 100) { 
		aet_ctrl->reset = 1;
		return;
	}
		
	if (aet_ctrl->reset == 0) { 
		printk("--------\n");
		printk("reset:%d hot_set_time:%d track_time:%lu add_to_hot_set_time:%d\n", aet_ctrl->reset, aet_ctrl->hot_set_time, aet_ctrl->track_fault_time, aet_ctrl->add_to_hot_set_time);
	}
	else
		return;

	
	/*
	if (aet_ctrl->hot_set_time > 5) { 
		aet_ctrl->reset = 1;
		return;
	}
	*/

	// for debug fake
	/*
	if (aet_ctrl->last_set_num > 3000) { 
		printk("last set num:%d\n", aet_ctrl->last_set_num);
		aet_ctrl->reset = 1;
		return;
	}
	*/

	aet_ctrl->reset = 2;
	//memset(aet_ctrl->hns_, 0, sizeof(aet_ctrl->hns_));
	if (aet_ctrl->hot_set_end != 0 && aet_ctrl->hot_set_time > aet_ctrl->hot_set_end) { 
		printk("reach hot set end not track anymore\n");
		return;
	}

	memset(aet_ctrl->hot_set, 0, sizeof(aet_ctrl->hot_set));
	aet_ctrl->hot_set_pos = 0;
	aet_ctrl->hot_set_time = 0;
	aet_ctrl->aet_func_num = 0;
	aet_ctrl->add_to_hot_set_num = 0;
	aet_ctrl->add_to_hot_set_time = 0;

	//clear lru list
	aet_ctrl->lru_list_pos = 0;
	aet_ctrl->lru_head_pointer->next = NULL; 

	aet_ctrl->set_sl1mfn_page_num++;	
	if (SAMPLE_FLAG)
		count = rand_track_algorithm(&hash_conflict);
	else
		count = full_track_algorithm(&hash_conflict);
	add_set_aet_magic_count(16, 0, 0, 0, hot_set_time);
	printk("%s count:%d set_sl1mfn_page_num:%lu\n", __func__, count, aet_ctrl->set_sl1mfn_page_num);
}

void add_vmexit_num(void) {
	aet_ctrl->vmexit_num++;
}

/* lru related function */
static int add_to_hist(int pos) {
    if (pos < LRU_MAX_PAGE_NUM) {
        aet_ctrl->lru_hist_[pos]++;
        return 0;
    }

    printk("pos:%d exceeds the maximum\n", pos);
    return 1;
}

static void add_to_lru_list_head(struct lru_node* ln) {
    if (ln->next != NULL)
        ln->next->last = ln->last;
    if (ln->last != NULL) 
        ln->last->next = ln->next;

    if (aet_ctrl->lru_head_pointer->next != NULL)
        aet_ctrl->lru_head_pointer->next->last = ln;
    ln->next = aet_ctrl->lru_head_pointer->next;
    aet_ctrl->lru_head_pointer->next = ln;
    ln->last = aet_ctrl->lru_head_pointer;
}

static struct lru_node* find_lru_node(unsigned long mfn, int *pos) {
    struct lru_node* ln = aet_ctrl->lru_head_pointer->next;
    *pos = 0;
    while (ln != NULL) {
        (*pos)++;
        if (ln->mfn == mfn) {
            return ln;
        }

        ln = ln->next;
    }

    return NULL;
}

static struct lru_node* add_lru_node(unsigned long mfn) {
    if (aet_ctrl->lru_list_pos < LRU_MAX_PAGE_NUM) {
        struct lru_node* new_node;
        new_node = &aet_ctrl->lru_list[aet_ctrl->lru_list_pos];
        new_node->mfn = mfn;
        new_node->last = NULL;
        new_node->next = NULL;
        aet_ctrl->lru_list_pos++;
        return new_node;
    }

	printk("exceeds maximum pos:%d\n", aet_ctrl->lru_list_pos);
    return NULL;
}

unsigned long alloc_shared_memory(unsigned long size, unsigned long va)
{
    unsigned long page_step, i, mfn, page_nr;
    l4_pgentry_t *pl4e;
    l3_pgentry_t *pl3e;
    l2_pgentry_t *pl2e;
    l1_pgentry_t *pl1e;

    page_nr = PFN_UP(size);

    page_step = (1<<L2_PAGETABLE_SHIFT) >> PAGE_SHIFT;

	printk("[joe] alloc_shared_memory page_nr:%lu page_step:%lx\n", page_nr, page_step);
    for(i=0; i<page_nr; i+= page_step)
    {
       	//printk("allocate %lu pages for lru_list, page_step=%lu\n", min(lru_list_page_nr-i, page_step), page_step);
       	mfn = alloc_boot_pages(min(page_nr - i, page_step), page_step);
//		printk("alloc boot pages:%lx\n", mfn);

       	if( mfn==0 )
       	    panic("No enought memory for lru list");

       	map_pages_to_xen(va + (i << PAGE_SHIFT), mfn, page_step, _PAGE_PRESENT | _PAGE_RW | _PAGE_USER |  _PAGE_DIRTY | _PAGE_ACCESSED);
    }

    //set user permission
    pl4e = &idle_pg_table[l4_table_offset(va)];
    l4e_add_flags(*pl4e, _PAGE_USER);
    pl3e = mfn_to_virt(l4e_get_pfn(*pl4e));
    l3e_add_flags(*pl3e, _PAGE_USER);
    pl2e = mfn_to_virt(l3e_get_pfn(*pl3e));
    pl1e = mfn_to_virt(l2e_get_pfn(*pl2e));

    memset((void*)va, 0, page_nr << PAGE_SHIFT);

    return page_nr;

}

void aet_init() {
	printk("[joe] in aet_init\n");
	aet_ctrl = (struct AET_ctrl*)SHARED_DATA_START;
	printk("[joe] aet_ctrl addr:%lx\n", (unsigned long)aet_ctrl);
    alloc_shared_memory(sizeof(struct AET_ctrl), (unsigned long)aet_ctrl);

    memset(aet_ctrl, 0, sizeof(aet_ctrl));
	aet_ctrl->hot_set_size = DEFAULT_HOT_SET_SIZE;
	aet_ctrl->hot_set_end = HOT_SET_END;
	aet_ctrl->lru_head.mfn = 0;

	aet_ctrl->lru_head_pointer = &(aet_ctrl->lru_head);
	aet_ctrl->lru_head_pointer->last = NULL;
	aet_ctrl->lru_head_pointer->next = NULL;

	randseed = (unsigned int) get_sec();
	aet_ctrl->lru_list = (struct lru_node*)LRU_LIST_VIRT_START;
	aet_ctrl->lru_length = LRU_MAX_PAGE_NUM;

	aet_ctrl->lru_node_page_nr = alloc_shared_memory((aet_ctrl->lru_length + 1) * (sizeof(struct lru_node)), LRU_LIST_VIRT_START);
	memset(aet_ctrl->lru_list, 0, aet_ctrl->lru_node_page_nr << PAGE_SHIFT);
	aet_ctrl->sleep = 0;
	aet_ctrl->add_to_all_sl1mfn_time = 0;
	printk("[joe] aet_init end lru_node_page_nr:%lu\n", aet_ctrl->lru_node_page_nr);
}


void set_aet_cmd(enum AET_CMD_OP aet_cmd, unsigned long arg1, unsigned long arg2) {
	printk("set aet cmd, AET_CMD = %s\n", AET_CMD_NAME[aet_cmd]);
	switch(aet_cmd) {
		case NO_OP:
			printk("no op\n");
			break;
		case SET_OPEN:
			printk("set track open :%s set track way:%s\n", OPEN_NAME[arg1], TRACK_NAME[arg2]);
			set_aet_track_open(arg1, arg2);
			break;
		case SET_TRACK:
			printk("set track way :%s\n", TRACK_NAME[arg1]);
			set_track(arg1);
			break;
		case SET_HOT_SET_SIZE:
			printk("modify hot set size:%lu\n", arg1);
			aet_ctrl->hot_set_size = arg1;
			break;	
		default:
			printk("invalid aet cmd\n");
	}
}
