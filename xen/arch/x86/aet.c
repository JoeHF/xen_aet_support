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

void add_set_aet_magic_count(unsigned long va, unsigned long l1, unsigned long l1p, unsigned long ec) {
	if (aet_ctrl->total_count + 10 < MAX_ARRAY_SIZE) {
		aet_ctrl->tvs[aet_ctrl->total_count].va = va;
		aet_ctrl->tvs[aet_ctrl->total_count].type = SET;
		aet_ctrl->tvs[aet_ctrl->total_count].l1 = l1;
		aet_ctrl->tvs[aet_ctrl->total_count].l1p = l1p;
		aet_ctrl->tvs[aet_ctrl->total_count].ec = ec;
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

void add_user_mode_fault_count(unsigned long va, unsigned long l1, unsigned long l1p, unsigned long ec) {
	aet_ctrl->user_mode_fault++;
	if (aet_ctrl->total_count + 10 < MAX_ARRAY_SIZE) {
		aet_ctrl->tvs[aet_ctrl->total_count].va = va;
		aet_ctrl->tvs[aet_ctrl->total_count].type = USER_MODE;
		aet_ctrl->tvs[aet_ctrl->total_count].l1 = l1;
		aet_ctrl->tvs[aet_ctrl->total_count].l1p = l1p;
		aet_ctrl->tvs[aet_ctrl->total_count].ec = ec;
		aet_ctrl->total_count++;
	}

}

void add_reserved_bit_fault_count(void) {
	aet_ctrl->reserved_bit_fault++;
}

void add_both_fault_count(void) {
	aet_ctrl->both_fault++;
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
