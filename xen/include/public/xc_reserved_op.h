#ifndef _PUBLIC_XC_RESERVED_OP_H
#define _PUBLIC_XC_RESERVED_OP_H

#define IA32_PMC0 0xC1
#define IA32_PERFEVTSEL0 0x186
#define IA32_PERF_GLOBAL_STATUS 0x38E
#define IA32_PERF_GLOBAL_CTRL 0x38F
#define IA32_PERF_GLOBAL_OVF_CTRL 0x390
#define IA32_PEBS 0x3F1
#define IA32_FIXED_CTR_CTRL 0x38D
#define IA32_FIXED_CTR 0x309
#define IA32_PEBS_ENABLE 0x3f1
#define MSR_PEBS_LD_LAT 0x3f6
#define IA32_DS_AREA 0x600
#define IA32_DEBUG_CTRL 0x1D9

#define CMASK 24
#define Invert_counter_mask 23
#define Enable_counters 22
#define Any_Thread 21
#define APIC_interrupt_enable 20
#define Pin_control 19
#define Edge_detect 18
#define OS 17
#define USR 16

#define PMC_N 4
#define PMC_N_MASK ((1 << PMC_N) - 1)
#define FIXED_N 3
#define FIXED_N_MASK ((((1UL) << FIXED_N) - 1) << 32)
#define FIXED_CONTROL (7)

unsigned long stop_pmu_return(int cpu, int arg2);
unsigned long pmu_l3_return(int cpu, int arg2);
unsigned long pmu_mem_return(int cpu, int arg2, unsigned long *mem, unsigned long *dtlb_load_miss, unsigned long *dtlb_store_miss);

#endif
