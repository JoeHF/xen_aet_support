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
static char* AET_CMD_NAME[3] = {"NO_OP", "SET_OPEN", "SET_TRACK"};
static char* TRACK_NAME[3] = {"L1_TRACK", "L4_TRACK", "PAGE_FAULT_COUNT"};
static char* OPEN_NAME[2] = {"CLOSED", "OPEN"};

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
		aet_ctrl->tvs[aet_ctrl->total_count].l1p = l1p;
		aet_ctrl->tvs[aet_ctrl->total_count].ec = ec;
		aet_ctrl->tvs[aet_ctrl->total_count].mc = mc;
		aet_ctrl->total_count++;
	}

	aet_ctrl->set_aet_magic_count++;
}

void add_reversed_aet_magic_count(unsigned long va, unsigned long l1) {
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
	if (aet_ctrl->total_count + 10 < MAX_ARRAY_SIZE) {
		aet_ctrl->tvs[aet_ctrl->total_count].va = va;
		aet_ctrl->tvs[aet_ctrl->total_count].type = USER_MODE;
		aet_ctrl->tvs[aet_ctrl->total_count].l1 = l1;
		aet_ctrl->tvs[aet_ctrl->total_count].l1p = l1p;
		aet_ctrl->tvs[aet_ctrl->total_count].ec = ec;
		aet_ctrl->tvs[aet_ctrl->total_count].mc = mc;
		
		aet_ctrl->total_count++;
	}

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
		printk("[WARNING] old mem counter is larger than the new mem counter old_mc:%lu new_mc:%lu\n", old_mc, new_mc);
		return;
	}

	compressed_dis = domain_value_to_index(new_mc - old_mc);	
	//printk("real dis:%lu compressed_dis:%lu\n", new_mc - old_mc, compressed_dis);
	if (compressed_dis >= MAX_PAGE_NUM) {
		printk("[WARNING] compressed dis is larger than array size new_mc:%lu old_mc:%lu compressed_dis:%lu\n", new_mc, old_mc, compressed_dis);
		return;
	}

	aet_ctrl->aet_hist_[domain_id - 1][compressed_dis]++;
}

unsigned long get_mem_counter(void) {
	unsigned long mc;
	unsigned long surplus_mc;
	mc = pmu_mem_return(1, 0);
	if (mc == 0)
		return 0;

	surplus_mc = aet_ctrl->surplus_mc;
	if (aet_ctrl->surplus_count2 != 0)
		surplus_mc += aet_ctrl->surplus_mc2 / aet_ctrl->surplus_count2 * aet_ctrl->vmexit_num;
	if (mc <= surplus_mc) {
		printk("[WARNING]%s mc sub surplus less than zero vmexit_num:%lu mc:%lu surplus_mc:%lu\n", __func__, aet_ctrl->vmexit_num, mc, surplus_mc);
		return mc;
	}

	return mc - surplus_mc;

}

void track_aet_fault(int domain_id, unsigned long mfn) {
	int key;
	int hash_pos = 0;
//	unsigned long page_fault_diff;
    unsigned long mc = get_mem_counter();
	unsigned long mc_diff = 0;
	key = mfn % HASH;
	for (hash_pos = 0 ; hash_pos < HASH_CONFLICT_NUM ; hash_pos++) {
		if (aet_ctrl->hns_[domain_id - 1][key][hash_pos].mfn == mfn) {
			//page_fault_diff = aet_ctrl->page_fault_count - aet_ctrl->hns_[domain_id - 1][key][hash_pos].pf; 
			/* sub the additional mem ref by shadow page fault */
			//mc_diff = page_fault_diff * 20;
			add_to_aet_histogram(domain_id, aet_ctrl->hns_[domain_id - 1][key][hash_pos].mem_counter, mc - mc_diff);

			add_user_mode_fault_count(mfn, 0, mc_diff, domain_value_to_index(mc - aet_ctrl->hns_[domain_id - 1][key][hash_pos].mem_counter), mc - aet_ctrl->hns_[domain_id - 1][key][hash_pos].mem_counter - mc_diff); // for debug	

			aet_ctrl->hns_[domain_id - 1][key][hash_pos].mem_counter = mc;
			aet_ctrl->hns_[domain_id - 1][key][hash_pos].pf = aet_ctrl->page_fault_count;
			aet_ctrl->tot_ref_[domain_id - 1]++;
			return;
		}

		if (aet_ctrl->hns_[domain_id - 1][key][hash_pos].mfn == 0) {
			printk("[WARNING] There is tracked aet fault but not in hash table mfn:%lx\n", mfn);
			aet_ctrl->hns_[domain_id - 1][key][hash_pos].mfn = mfn;
			aet_ctrl->hns_[domain_id - 1][key][hash_pos].mem_counter = mc;
			aet_ctrl->hns_[domain_id - 1][key][hash_pos].pf = aet_ctrl->page_fault_count;
			aet_ctrl->node_count_[domain_id - 1]++;
			return;
		}
	}

	aet_ctrl->hash_conflict_num++;
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
	unsigned long va_start = va - (va & L1_MASK & PAGE_MASK);
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
	aet_ctrl->pds[aet_ctrl->set_num].va = va_start;
	aet_ctrl->set_num++;

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
static inline mfn_t shadow_l1e_get_mfn(shadow_l1e_t sl1e)
{ return _mfn(l1e_get_pfn(sl1e)); }

static void set_page_mc(int domain_id, unsigned long mfn);
static void track_l1_page(unsigned long sl1mfn, unsigned long va, unsigned long *count, int set) {
	shadow_l1e_t *sp, *sl1e;
	int i;
	unsigned long user_bit = 0x4;
	unsigned long mc;
	sp = map_domain_page(sl1mfn);
	mc = get_mem_counter();
	if (mfn_to_page(sl1mfn)->u.sh.type == SH_type_l1_shadow
		|| mfn_to_page(sl1mfn)->u.sh.type == SH_type_fl1_shadow) {
		for (i = 0 ; i < CONSECUTIVE_SET_PAGE ; i++) {
			sl1e = sp + i;
			if ((shadow_l1e_get_flags(*sl1e) & _PAGE_USER)
				&& (shadow_l1e_get_flags(*sl1e) & _PAGE_RW)
				&& (shadow_l1e_get_flags(*sl1e) & _PAGE_PRESENT)
				&& (sl1e->l1 & SH_L1E_AET_MAGIC) == 0) {
					if (set) {
						(*count)++;
					//	printk("[joe]%s set aet magic va:%lx sl1mfn:%lx count:%lu\n", __func__, va, sl1mfn, *count);
						add_set_aet_magic_count(va, sl1e->l1, sl1mfn, 1, mc);
						set_page_mc(1, ((mfn_x(shadow_l1e_get_mfn(*sl1e))) & 0x7fffffffff));
						sl1e->l1 |= SH_L1E_AET_MAGIC;
						sl1e->l1 &= (~user_bit);
						//flush_tlb_one_local(va);
					}
			}
			
			va += (1 << 12);
		}
	}	
	//else {
	//	printk("[WARNING]%s sl1mfn:%lx va:%lx is not a l1 shadow page\n", __func__, sl1mfn, va);
	//}
}

void set_pending_page() {
	int j;
	unsigned long sl1mfn;
	unsigned long va;
	unsigned long count = 0;
	if (aet_ctrl->set_num == 0)
		return;
	//else
	//	printk("[joe]%s set_num:%lu\n", __func__, aet_ctrl->set_num);

	for (j = 0 ; j < aet_ctrl->set_num ; j++) {
		sl1mfn = aet_ctrl->pds[j].sl1mfn;
		va = aet_ctrl->pds[j].va;
		add_set_aet_magic_count(va, 0, sl1mfn, 2, 0);
		track_l1_page(sl1mfn, va, &count, 0);
	}	

	aet_ctrl->set_num = 0;
	flush_tlb_local();
	//if (count != 0)
	//	printk("[joe]%s set %lu page\n", __func__, count);
}

/* The following function is used by add page to track page set
 * by issue 15
 */
void add_to_track_page_set(unsigned long sl1mfn, unsigned long va) {
	int i;
	unsigned long va_start = va - (va & L1_MASK & PAGE_MASK);
	if (aet_ctrl->tracking_page_set_num >= MAX_TRACKING_PAGE) {
		printk("[WARNING]%s tracking page set num exceeds the max tracking page num:%d\n", __func__, MAX_TRACKING_PAGE);
		return;
	}

	for (i = 0 ; i < aet_ctrl->tracking_page_set_num ; i++) {
		if (aet_ctrl->tracking_page_set_[i].sl1mfn == sl1mfn)
			return;
	}

	add_set_aet_magic_count(va_start, 0, sl1mfn, 4, 0);
	aet_ctrl->tracking_page_set_[aet_ctrl->tracking_page_set_num].sl1mfn = sl1mfn;
	aet_ctrl->tracking_page_set_[aet_ctrl->tracking_page_set_num].va = va_start;
	aet_ctrl->tracking_page_set_num++;
}

static void set_page_mc(int domain_id, unsigned long mfn) {
	int key;
	int hash_pos = 0;
	unsigned long mc;
	mc = get_mem_counter();	
	key = mfn % HASH;
	for (hash_pos = 0 ; hash_pos < HASH_CONFLICT_NUM ; hash_pos++) {
		if (aet_ctrl->hns_[domain_id - 1][key][hash_pos].mfn == mfn) {
			aet_ctrl->hns_[domain_id - 1][key][hash_pos].mem_counter = mc;
			return;
		}

		else if (aet_ctrl->hns_[domain_id - 1][key][hash_pos].mfn == 0) {
			aet_ctrl->hns_[domain_id - 1][key][hash_pos].mem_counter = mc;
			aet_ctrl->hns_[domain_id - 1][key][hash_pos].mfn = mfn;
			aet_ctrl->node_count_[domain_id - 1]++;
			return;
		}
	}

	aet_ctrl->hash_conflict_num++;
	
}

void set_all_track_page() {
	int i;
	unsigned long sl1mfn;
	unsigned long va;
	unsigned long count = 0;
//	unsigned long mc;
//	mc = pmu_mem_return(1, 0);
//	add_set_aet_magic_count(0, 0, 0, 0, mc);
	if (aet_ctrl->tracking_page_set_num == 0)
		return;
	for (i = 0 ; i < aet_ctrl->tracking_page_set_num ; i++) {
		sl1mfn = aet_ctrl->tracking_page_set_[i].sl1mfn;
		va = aet_ctrl->tracking_page_set_[i].va;
		track_l1_page(sl1mfn, va, &count, 1);
	}	

	// for debug
	if (count != 0)
		printk("[joe]%s set:%lu/%lu track page\n", __func__, count, aet_ctrl->tracking_page_set_num * CONSECUTIVE_SET_PAGE);
//	aet_ctrl->tracking_page_set_num = 0;
}

void add_vmexit_num() {
	aet_ctrl->vmexit_num++;
}

void add_surplus_mc(unsigned long emc, unsigned long omc) {
	aet_ctrl->surplus_count += 1;
	aet_ctrl->surplus_mc += (omc - emc);
//	if (aet_ctrl->surplus_count != 0)
//		printk("[joe]%s, diff:%lu avg:%lu\n", __func__, omc - emc, aet_ctrl->surplus_mc / aet_ctrl->surplus_count);
}

void add_surplus_mc2(unsigned long emc, unsigned long omc) {
	aet_ctrl->surplus_count2 += 1;
	aet_ctrl->surplus_mc2 += (omc - emc);
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
