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
static char* AET_CMD_NAME[3] = {"NO_OP", "SET_OPEN", "SET_TRACK"};
static char* TRACK_NAME[3] = {"L1_TRACK", "L4_TRACK", "PAGE_FAULT_COUNT"};
static char* OPEN_NAME[2] = {"CLOSED", "OPEN"};

static unsigned int curl_rand(void) {
	unsigned int r;
	r = randseed = randseed * 1103515245 + 12345;
	return (r << 16) | ((r >> 16) & 0xFFFF);
}
                                                                                                           
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

int l1_set_over(int count) {
	return count >= CONSECUTIVE_SET_PAGE;
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

void add_tracked_aet_magic_count1(void) {
	aet_ctrl->tracked_aet_magic_count1++;
}

void add_tracked_aet_magic_count2(void) {
	aet_ctrl->tracked_aet_magic_count2++;
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

static void add_to_aet_histogram(int domain_id, unsigned long old_mc, unsigned long new_mc) {
	unsigned long compressed_dis;
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

	if (compressed_dis > aet_ctrl->longest_aet_hist_pos[domain_id - 1]) { 
		aet_ctrl->longest_aet_hist_pos[domain_id - 1] = compressed_dis;
	}

	aet_ctrl->aet_hist_[domain_id - 1][compressed_dis]++;
}

static void add_to_aet_first_hist2(int domain_id,
								   unsigned long old_dtlb,
								   unsigned long new_dtlb,
								   unsigned long old_mem,
								   unsigned long new_mem) { 
	unsigned long dtlb_miss_diff;
	unsigned long mem_diff;
	unsigned long surplus = 0;
	if (old_dtlb >= new_dtlb) { 
		printk("[WARNING] old dtlb counter:%lu is larger than the new dtlb counter:%lu\n", old_dtlb, new_dtlb);
		return;
	}	

	if (old_mem > new_mem) {
		printk("[WARNING] old mem counter:%lu is larger than the new mem counter:%lu\n", old_mem, new_mem);
		return;
	}

	dtlb_miss_diff = new_dtlb - old_dtlb;
	//dtlb_miss_diff -= aet_ctrl->node_count_[domain_id - 1];
//	dtlb_miss_diff *= DTLB_ENTRY;
	mem_diff = new_mem - old_mem;
	if (mem_diff <= dtlb_miss_diff) { 
		printk("[WARNING] mem_diff:%lu is less than dtlb_miss_diff:%lu\n", mem_diff, dtlb_miss_diff);
		return;
	}

	aet_ctrl->surplus_total += mem_diff - dtlb_miss_diff;
	aet_ctrl->surplus_time++;
	if (aet_ctrl->valid_sl1mfn[domain_id - 1] != 0)
		surplus = (mem_diff - dtlb_miss_diff) / (aet_ctrl->valid_sl1mfn[domain_id - 1] * 512);
	else
		printk("[WARNING] valid sl1mfn is zero\n");
	add_user_mode_fault_count(10, 0, 0, 0, surplus); // for debug
	aet_ctrl->aet_hist_[domain_id - 1][1] += surplus;
	aet_ctrl->tot_ref_[domain_id - 1] += surplus;
}

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

int track_aet_fault(int domain_id, 
					 unsigned long mfn, 
					 unsigned long mem_counter, 
					 unsigned long dtlb_load_miss,
					 unsigned long dtlb_store_miss,
					 unsigned long sl1mfn) {
	int i;
	int key;
	int hash_pos = 0;
	unsigned long page_fault_diff;
	unsigned long mc_diff;
	//unsigned long real_dtlb_miss_diff;
	unsigned long dtlb_miss;
	struct hash_node *hn;
	aet_ctrl->mem_now = mem_counter;
	key = mfn % HASH;
	dtlb_miss = dtlb_load_miss + dtlb_store_miss;
	for (hash_pos = 0 ; hash_pos < HASH_CONFLICT_NUM ; hash_pos++) {
		hn = aet_ctrl->hns_[domain_id - 1][key] + hash_pos;
		if (hn->mfn == mfn) {
			page_fault_diff = aet_ctrl->page_fault_count - hn->pf; 
			/* sub the additional mem ref by shadow page fault */
			mc_diff = page_fault_diff * 20;
			mc_diff = 0;

			if (mem_counter > hn->mem_counter)
				add_user_mode_fault_count(mfn, 0, mc_diff, domain_value_to_index(mem_counter - hn->mem_counter), mem_counter - hn->mem_counter - mc_diff); // for debug	
			else
				printk("[WARNING]%s old mem%lu is larger than new mem:%lu\n", __func__, hn->mem_counter, mem_counter);
			
			if (hn->track_time == 1) { 
				add_to_aet_histogram(domain_id, hn->mem_counter, mem_counter - mc_diff);

				add_tlb_counter(hn->track_time, hn->dtlb_miss, dtlb_miss); // for debug
				add_mem_counter(hn->track_time, hn->mem_counter, mem_counter); // for debug
				add_diff(hn->track_time, dtlb_miss - hn->dtlb_miss, mem_counter - hn->mem_counter - mc_diff);
				
				//printk("valid sl1mfn:%lu\n", aet_ctrl->valid_sl1mfn[0]);
				add_user_mode_fault_count(11, 0, 0, aet_ctrl->node_count_[0], (unsigned long)aet_ctrl->valid_sl1mfn[0]); // for debug	
				add_to_aet_first_hist2(domain_id, hn->dtlb_miss, dtlb_miss, hn->mem_counter, mem_counter);
			}

			hn->mem_counter = mem_counter;
			hn->dtlb_miss = dtlb_miss;
			hn->dtlb_load_miss = dtlb_load_miss;
			hn->dtlb_store_miss = dtlb_store_miss;
			hn->pf = aet_ctrl->page_fault_count;
			hn->track_time++;
			aet_ctrl->tot_ref_[domain_id - 1]++;
			if (hn->track_time >= 2) { 
				hn->track_time = 0;
				//aet_ctrl->valid_node_count[domain_id - 1]--;
				
				for (i = 0 ; i < aet_ctrl->sl1mfn_num ; i++) { 
					if (aet_ctrl->all_sl1mfn[i].sl1mfn == sl1mfn) { 
						if (aet_ctrl->all_sl1mfn[i].track_num <= 0) { 
							printk("[Warning] sl1mfn track num less than zero\n");
						}
						aet_ctrl->all_sl1mfn[i].track_num--;
						if (aet_ctrl->all_sl1mfn[i].track_num == 0)
							aet_ctrl->valid_sl1mfn[0]--;

						break;
					}
				}
				
				return 0;
			}

			return 1;
		}

		if (hn->mfn == 0) {
			hn->mfn = mfn;
			hn->mem_counter = mem_counter;
			hn->dtlb_load_miss = dtlb_load_miss;
			hn->dtlb_store_miss = dtlb_store_miss;
			hn->dtlb_miss = dtlb_miss;
			hn->pf = aet_ctrl->page_fault_count;
			hn->track_time = 1;
			aet_ctrl->node_count_[domain_id - 1]++;
			return 1;
		}
	}

	aet_ctrl->hash_conflict_num++;
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
void add_to_pending_page(unsigned long sl1mfn, unsigned long va) {
	//unsigned long va_start = va - (va & L1_MASK & PAGE_MASK);
	int i;
	// remove duplicate one
	for (i = aet_ctrl->set_num - 1 ; i >= 0 ; i--) {
		if (aet_ctrl->pds[i].sl1mfn == sl1mfn)
			return;
	}

	if (aet_ctrl->set_num >= MAX_PENDING_PAGE) {
		printk("[joe]%s exceed MAX_PENDING_PAGE_SIZE\n", __func__);
		return;
	}

	aet_ctrl->pds[aet_ctrl->set_num].sl1mfn = sl1mfn;
	aet_ctrl->pds[aet_ctrl->set_num].va = va;
	aet_ctrl->set_num++;

}

void add_to_all_sl1mfn(unsigned long sl1mfn, unsigned long va) {
	unsigned long va_start = va - (va & L1_MASK & PAGE_MASK);
	int i;
	//printk("%s sl1mfn:%lx, va:%lx\n", __func__, sl1mfn, va);
	// remove duplicate one
	for (i = aet_ctrl->sl1mfn_num - 1 ; i >= 0 ; i--) {
		if (aet_ctrl->all_sl1mfn[i].sl1mfn == sl1mfn)
			return;
	}

	if (aet_ctrl->sl1mfn_num >= MAX_PENDING_PAGE) {
		printk("[joe]%s sl1mfn num exceed MAX_PENDING_PAGE_SIZE\n", __func__);
		return;
	}

	aet_ctrl->all_sl1mfn[aet_ctrl->sl1mfn_num].sl1mfn = sl1mfn;
	aet_ctrl->all_sl1mfn[aet_ctrl->sl1mfn_num].va = va_start;
	aet_ctrl->sl1mfn_num++;

}

/* steal from shadow code */
typedef l1_pgentry_t shadow_l1e_t;
#define SH_type_l1_64_shadow   (8U) /* shadowing a 64-bit L1 page */
#define SH_type_fl1_64_shadow  (9U) /* L1 shadow for 64-bit 2M superpg */
#define SH_type_l1_shadow  SH_type_l1_64_shadow
#define SH_type_fl1_shadow SH_type_fl1_64_shadow
#define SH_L1E_AET_MAGIC 0x7ff8000000000400ULL
static inline u32 shadow_l1e_get_flags(shadow_l1e_t sl1e)
{ return l1e_get_flags(sl1e); }

void rand_track_page() { 
	int i;
	int j;
	unsigned long sl1mfn, va;
	unsigned long user_bit = 0x4;
	unsigned long access_bit = 0x20;
	//int rands[5] = {3, 7, 20, 50, 511};
	int rand_pos = curl_rand() % 512;
	unsigned long count = 0;
	shadow_l1e_t *sp, *sl1e;
	//printk("%s sl1mfn_num:%d\n", __func__, aet_ctrl->sl1mfn_num);
	if (aet_ctrl->sl1mfn_num == 0) { 
		return;
	}

	//rand_pos = 0;
	aet_ctrl->set_sl1mfn_page_num++;	
	aet_ctrl->valid_sl1mfn[0] = 0;
	for (i = 0 ; i < aet_ctrl->sl1mfn_num ; i++) { 
		if (aet_ctrl->all_sl1mfn[i].track_num >= CONSECUTIVE_SET_PAGE) { 
			//printk("already enough track sl1mfn\n");
			aet_ctrl->valid_sl1mfn[0]++;
			continue;
		}
		sl1mfn = aet_ctrl->all_sl1mfn[i].sl1mfn;
		va = aet_ctrl->all_sl1mfn[i].va;
		//add_set_aet_magic_count(va, 0, sl1mfn, 4, 0);
		if (mfn_to_page(sl1mfn)->u.sh.type == SH_type_l1_shadow
			|| mfn_to_page(sl1mfn)->u.sh.type == SH_type_fl1_shadow) {
			sp = map_domain_page(sl1mfn);
			//while (1) { 
				for (j = 0 ; j < CONSECUTIVE_SET_PAGE ; j++) { 
					sl1e = sp + rand_pos;	
					//add_set_aet_magic_count(va, 0, sl1mfn, 4, rand_pos);
					if ((shadow_l1e_get_flags(*sl1e) & _PAGE_USER)
						&& (shadow_l1e_get_flags(*sl1e) & _PAGE_RW)
						&& (shadow_l1e_get_flags(*sl1e) & _PAGE_PRESENT)
						&& (sl1e->l1 & SH_L1E_AET_MAGIC) == 0) {
							if (aet_ctrl->all_sl1mfn[i].track_num == 0)
								aet_ctrl->valid_sl1mfn[0]++;
							count++;
							sl1e->l1 |= SH_L1E_AET_MAGIC;
							sl1e->l1 &= (~user_bit);
							sl1e->l1 &= (~access_bit);
							//aet_ctrl->valid_node_count[0]++;
							add_set_aet_magic_count(va, sl1e->l1, (unsigned long)sl1e, 3, 0);
							aet_ctrl->all_sl1mfn[i].track_num++;
							if (aet_ctrl->all_sl1mfn[i].track_num >= CONSECUTIVE_SET_PAGE)
								break;
					}
	//				else { 
	//					j--;
	//				}

					//rand_pos++;
					rand_pos = rand_pos + curl_rand() % 512;
					if (rand_pos >= 512) { 
						rand_pos %= 512;
					//	break;
					}
					//break;
				}
			//}
		}
		else { 
	//		printk("[WARNING]%s sl1mfn:%lx va:%lx is not a l1 shadow page\n", __func__, sl1mfn, va);
		}
	}

	//add_set_aet_magic_count(aet_ctrl->valid_sl1mfn[0], 0, 0, 11, aet_ctrl->valid_node_count[0]);
	add_set_aet_magic_count(aet_ctrl->valid_sl1mfn[0], 0, 0, 12, aet_ctrl->node_count_[0]);
	//printk("%s %lu/%d valid:%lu\n", __func__, count, aet_ctrl->sl1mfn_num, aet_ctrl->valid_sl1mfn[0]);
	//printk("node count:%lu valid node count:%lu\n", aet_ctrl->valid_node_count[0], aet_ctrl->node_count_[0]);
}


void set_pending_page() {
	//int i;
	int j;
	unsigned long sl1mfn;
	shadow_l1e_t *sl1e;
	unsigned long va;
	unsigned long count = 0;
	unsigned long user_bit = 0x4;
	unsigned long access_bit = 0x20;
	unsigned long mc = 0;
	unsigned long va_start;
	unsigned long va_offset;
	//unsigned long mem, dtlb_load_miss, dtlb_store_miss;
	if (aet_ctrl->set_num == 0)
		return;
	//else
	//	printk("[joe]%s set_num:%lu\n", __func__, aet_ctrl->set_num);

	//printk("%s before set pending page\n", __func__);
	//mc = pmu_mem_return(1, 0, &mem, &dtlb_load_miss, &dtlb_store_miss);
	aet_ctrl->set_pending_page_num++;
	for (j = 0 ; j < aet_ctrl->set_num ; j++) {
		shadow_l1e_t *sp;
		sl1mfn = aet_ctrl->pds[j].sl1mfn;
		//printk("[joe]%s sl1mfn:%lx\n", __func__, sl1mfn);
		sp = map_domain_page(sl1mfn);
		va = aet_ctrl->pds[j].va;
		va_start = va - (va & L1_MASK & PAGE_MASK);
		va_offset = ((va & L1_MASK & PAGE_MASK) >> 12);
		//add_set_aet_magic_count(va, 0, 0, 6, va_offset);
		if (mfn_to_page(sl1mfn)->u.sh.type == SH_type_l1_shadow
			|| mfn_to_page(sl1mfn)->u.sh.type == SH_type_fl1_shadow) {
				sl1e = sp + va_offset;
				if ((shadow_l1e_get_flags(*sl1e) & _PAGE_USER)
					&& (shadow_l1e_get_flags(*sl1e) & _PAGE_RW)
					&& (shadow_l1e_get_flags(*sl1e) & _PAGE_PRESENT)
					&& (sl1e->l1 & SH_L1E_AET_MAGIC) == 0) {
						count++;
						sl1e->l1 |= SH_L1E_AET_MAGIC;
						sl1e->l1 &= (~user_bit);
						sl1e->l1 &= (~access_bit);
						add_set_aet_magic_count(va, sl1e->l1, (unsigned long)sl1e, 2, mc);
						//printk("%s set va:%lx, sl1e:%lx vmexit:%lu\n", __func__, va, sl1e->l1, aet_ctrl->vmexit_num);
						//flush_tlb_one_local(va);
						//flush_tlb_local();
				}
				
				va += (1 << 12);
		}	
		else {
			printk("[WARNING]%s sl1mfn:%lx va:%lx is not a l1 shadow page\n", __func__, sl1mfn, va);
		}

		unmap_domain_page(sp);
	}

	//printk("%s set aet magic num:%lu\n", __func__, aet_ctrl->set_num);
	aet_ctrl->set_num = 0;
	//if (count != 0)
	//	flush_tlb_local();
	//if (count != 0)
	//	printk("[joe]%s set %lu page\n", __func__, count);
}

void add_vmexit_num(void) {
	aet_ctrl->vmexit_num++;
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
	randseed = (unsigned int) get_sec();
	printk("[joe] aet_init end\n");
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
		default:
			printk("invalid aet cmd\n");
	}
}
