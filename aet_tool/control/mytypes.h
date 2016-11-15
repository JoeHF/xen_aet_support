typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

typedef __signed__ long long __s64;
typedef unsigned long long __u64;
typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

struct list_head {
 struct list_head *next, *prev;
};

#define L0_PAGETABLE_MASK_PAE	0x000ffffffffff000ULL
#define L1_PAGETABLE_MASK_PAE	0x1ffULL
#define L2_PAGETABLE_MASK_PAE	0x1ffULL
#define L3_PAGETABLE_MASK_PAE	0x1ffULL
#define L4_PAGETABLE_MASK_PAE	0x1ffULL
#define PAGETABLE_MFN_MASK	0xfffffUL

#define PT_ENTRY_NR (PAGE_SIZE / sizeof(long))

#define _PAGE_PRESENT   0x001
#define _PAGE_RW        0x002
#define _PAGE_USER      0x004
#define _PAGE_PWT       0x008
#define _PAGE_PCD       0x010
#define _PAGE_ACCESSED  0x020
#define _PAGE_DIRTY     0x040
#define _PAGE_PSE       0x080   /* 2MB page */
#define _PAGE_FILE      0x040   /* nonlinear file mapping, saved PTE; unset:swap */
#define _PAGE_GLOBAL    0x100   /* Global TLB entry */
#define _PAGE_PROTNONE  0x080   /* If not present */
struct rb_node
{
 unsigned long rb_parent_color;
 struct rb_node *rb_right;
 struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));

typedef struct {
 volatile unsigned int slock;
} raw_spinlock_t;


typedef struct {
 raw_spinlock_t raw_lock;
} spinlock_t;

typedef struct {
 volatile unsigned int lock;
} raw_rwlock_t;

typedef struct {
 raw_rwlock_t raw_lock;
} rwlock_t;


struct rw_semaphore {
 __s32 activity;
 spinlock_t wait_lock;
 struct list_head wait_list;
};
struct rb_root
{
 struct rb_node *rb_node;
};
struct hlist_head {
 struct hlist_node *first;
};

struct hlist_node {
 struct hlist_node *next, **pprev;
};


typedef struct { volatile long counter; } atomic64_t;
typedef struct { volatile int counter; } atomic_t;
typedef atomic64_t atomic_long_t;

typedef atomic_long_t mm_counter_t;

typedef struct { unsigned long pte; } pte_t;
typedef struct { unsigned long pmd; } pmd_t;
typedef struct { unsigned long pud; } pud_t;
typedef struct { unsigned long pgd; } pgd_t;
typedef struct { unsigned long bits[(((16)+64 -1)/64)]; } cpumask_t;

typedef struct __wait_queue_head wait_queue_head_t;
struct __wait_queue_head {
 spinlock_t lock;
 struct list_head task_list;
};
struct completion {
 unsigned int done;
 wait_queue_head_t wait;
};
typedef struct __wait_queue wait_queue_t;

struct semaphore {
 atomic_t count;
 int sleepers;
 wait_queue_head_t wait;
};

typedef struct {
 void *ldt;
 rwlock_t ldtlock;
 int size;
 struct semaphore sem;

 unsigned pinned:1;
 unsigned has_foreign_mappings:1;
 struct list_head unpinned;

} mm_context_t;

struct mm_struct {
 struct vm_area_struct * mmap;
 struct rb_root mm_rb;
 struct vm_area_struct * mmap_cache;
 unsigned long (*get_unmapped_area) (void *filp,
    unsigned long addr, unsigned long len,
    unsigned long pgoff, unsigned long flags);
 void (*unmap_area) (struct mm_struct *mm, unsigned long addr);
 unsigned long mmap_base;
 unsigned long task_size;
 unsigned long cached_hole_size;
 unsigned long free_area_cache;
 pgd_t * pgd;
 atomic_t mm_users;
 atomic_t mm_count;
 int map_count;
 struct rw_semaphore mmap_sem;
 spinlock_t page_table_lock;

 struct list_head mmlist;



 mm_counter_t _file_rss;
 mm_counter_t _anon_rss;

 unsigned long hiwater_rss;
 unsigned long hiwater_vm;

 unsigned long total_vm, locked_vm, shared_vm, exec_vm;
 unsigned long stack_vm, reserved_vm, def_flags, nr_ptes;
 unsigned long start_code, end_code, start_data, end_data;
 unsigned long start_brk, brk, start_stack;
 unsigned long arg_start, arg_end, env_start, env_end;

 unsigned long saved_auxv[44];

 unsigned dumpable:2;
 cpumask_t cpu_vm_mask;


 mm_context_t context;


 unsigned long swap_token_time;
 char recent_pagein;


 int core_waiters;
 struct completion *core_startup_done, core_done;


 rwlock_t ioctx_list_lock;
 struct kioctx *ioctx_list;
};

typedef void __signalfn_t(int);
typedef __signalfn_t *__sighandler_t;

typedef void __restorefn_t(void);
typedef __restorefn_t *__sigrestore_t;
struct sigaction {
 __sighandler_t sa_handler;
 unsigned long sa_flags;
 __sigrestore_t sa_restorer;
 sigset_t sa_mask;
};

struct k_sigaction {
 struct sigaction sa;
};
struct sighand_struct {
 atomic_t count;
 struct k_sigaction action[64];
 spinlock_t siglock;
};
typedef unsigned long cputime_t;

enum sleep_type {
 SLEEP_NORMAL,
 SLEEP_NONINTERACTIVE,
 SLEEP_INTERACTIVE,
 SLEEP_INTERRUPTED,
};
enum pid_type
{
 PIDTYPE_PID,
 PIDTYPE_PGID,
 PIDTYPE_SID,
 PIDTYPE_MAX
};
typedef __u32 kernel_cap_t;
struct sem_undo_list {
 atomic_t refcnt;
 spinlock_t lock;
 struct sem_undo *proc_list;
};

struct sysv_sem {
 struct sem_undo_list *undo_list;
};

struct i387_fxsave_struct {
 u16 cwd;
 u16 swd;
 u16 twd;
 u16 fop;
 u64 rip;
 u64 rdp;
 u32 mxcsr;
 u32 mxcsr_mask;
 u32 st_space[32];
 u32 xmm_space[64];
 u32 padding[24];
} __attribute__ ((aligned (16)));

union i387_union {
 struct i387_fxsave_struct fxsave;
};
struct plist_head {
 struct list_head prio_list;
 struct list_head node_list;
};

struct thread_struct {
 unsigned long rsp0;
 unsigned long rsp;
 unsigned long userrsp;
 unsigned long fs;
 unsigned long gs;
 unsigned short es, ds, fsindex, gsindex;

 unsigned long debugreg0;
 unsigned long debugreg1;
 unsigned long debugreg2;
 unsigned long debugreg3;
 unsigned long debugreg6;
 unsigned long debugreg7;

 unsigned long cr2, trap_no, error_code;

 union i387_union i387 __attribute__((aligned(16)));


 int ioperm;
 unsigned long *io_bitmap_ptr;
 unsigned io_bitmap_max;

 u64 tls_array[3];
 unsigned int iopl;
} __attribute__((aligned(16)));

typedef union sigval {
 int sival_int;
 void *sival_ptr;
} sigval_t;

typedef struct siginfo {
 int si_signo;
 int si_errno;
 int si_code;

 union {
  int _pad[((128 - (4 * sizeof(int))) / sizeof(int))];


  struct {
   pid_t _pid;
   uid_t _uid;
  } _kill;


  struct {
   timer_t _tid;
   int _overrun;
   char _pad[sizeof( uid_t) - sizeof(int)];
   sigval_t _sigval;
   int _sys_private;
  } _timer;


  struct {
   pid_t _pid;
   uid_t _uid;
   sigval_t _sigval;
  } _rt;


  struct {
   pid_t _pid;
   uid_t _uid;
   int _status;
   clock_t _utime;
   clock_t _stime;
  } _sigchld;


  struct {
   void *_addr;



  } _sigfault;


  struct {
   long _band;
   int _fd;
  } _sigpoll;
 } _sifields;
} siginfo_t;

struct sigpending {
 struct list_head list;
 sigset_t signal;
};
typedef struct { int mode; } seccomp_t;

struct rcu_head {
 struct rcu_head *next;
 void (*func)(struct rcu_head *head);
};
struct pid_link
{
 struct hlist_node node;
 struct pid *pid;
};

struct task_struct {
 volatile long state;
 struct thread_info *thread_info;
 atomic_t usage;
 unsigned long flags;
 unsigned long ptrace;
 int lock_depth;
 int load_weight;
 int prio, static_prio, normal_prio;
 struct list_head run_list;
 struct prio_array *array;
 unsigned short ioprio;
 unsigned int btrace_seq;
 unsigned long sleep_avg;
 unsigned long long timestamp, last_ran;
 unsigned long long sched_time;
 enum sleep_type sleep_type;
 unsigned long policy;
 cpumask_t cpus_allowed;
 unsigned int time_slice, first_time_slice;
 struct list_head tasks;
 struct list_head ptrace_children;
 struct list_head ptrace_list;
 struct mm_struct *mm, *active_mm;
 struct linux_binfmt *binfmt;
 long exit_state;
 int exit_code, exit_signal;
 int pdeath_signal;
 unsigned long personality;
 unsigned did_exec:1;
 pid_t pid;
 pid_t tgid;
 struct task_struct *real_parent;
 struct task_struct *parent;
 struct list_head children;
 struct list_head sibling;
 struct task_struct *group_leader;
 struct pid_link pids[PIDTYPE_MAX];
 struct list_head thread_group;
 struct completion *vfork_done;
 int *set_child_tid;
 int *clear_child_tid;
 unsigned long rt_priority;
 cputime_t utime, stime;
 unsigned long nvcsw, nivcsw;
 struct timespec start_time;
 unsigned long min_flt, maj_flt;
   cputime_t it_prof_expires, it_virt_expires;
 unsigned long long it_sched_expires;
 struct list_head cpu_timers[3];
 uid_t uid,euid,suid,fsuid;
 gid_t gid,egid,sgid,fsgid;
 struct group_info *group_info;
 kernel_cap_t cap_effective, cap_inheritable, cap_permitted;
 unsigned keep_capabilities:1;
 struct user_struct *user;
 int oomkilladj;
 char comm[16];
 int link_count, total_link_count;
 struct sysv_sem sysvsem;
 struct thread_struct thread;
 struct fs_struct *fs;
 struct files_struct *files;
 struct namespace *namespace;
 struct signal_struct *signal;
 struct sighand_struct *sighand;
 sigset_t blocked, real_blocked;
 sigset_t saved_sigmask;
 struct sigpending pending;
 unsigned long sas_ss_sp;
 size_t sas_ss_size;
 int (*notifier)(void *priv);
 void *notifier_data;
 sigset_t *notifier_mask;
 void *security;
 struct audit_context *audit_context;
 seccomp_t seccomp;
    u32 parent_exec_id;
    u32 self_exec_id;
 spinlock_t alloc_lock;
 spinlock_t pi_lock;
 struct plist_head pi_waiters;
 struct rt_mutex_waiter *pi_blocked_on;
 void *journal_info;
 struct reclaim_state *reclaim_state;
struct backing_dev_info *backing_dev_info;
 struct io_context *io_context;
 unsigned long ptrace_message;
 siginfo_t *last_siginfo;
 wait_queue_t *io_wait;
 u64 rchar, wchar, syscr, syscw;
 u64 acct_rss_mem1;
 u64 acct_vm_mem1;
 clock_t acct_stimexpd;
 struct robust_list_head *robust_list;
 struct compat_robust_list_head *compat_robust_list;
 struct list_head pi_state_list;
 struct futex_pi_state *pi_state_cache;
 atomic_t fs_excl;
 struct rcu_head rcu;
 struct pipe_inode_info *splice_pipe;
};


/**
  * container_of - cast a member of a structure out to the containing structure
  * @ptr:        the pointer to the member.
  * @type:       the type of the container struct this is embedded in.
  * @member:     the name of the member within the struct.
  *
  */
 #define container_of(ptr, type, member) ({                      \
         const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
         (type *)( (char *)__mptr - offsetof(type,member) );})
 
#define list_entry(ptr, type, member) \
         container_of(ptr, type, member)
 
 /**
  * list_for_each        -       iterate over a list
  * @pos:        the &struct list_head to use as a loop cursor.
  * @head:       the head for your list.
  */
 #define list_for_each(pos, head) \
         for (pos = (head)->next; prefetch(pos->next), pos != (head); \
                 pos = pos->next)

#define smp_read_barrier_depends()      do {} while(0)
#define rcu_dereference(p)     ({ \
                                typeof(p) _________p1 = p; \
                                smp_read_barrier_depends(); \
                                (_________p1); \
                                })
#define next_task(p)    list_entry(rcu_dereference((p)->tasks.next), struct task_struct, tasks)

#define for_each_process(p) \
        for (p = &init_task ; (p = next_task(p)) != &init_task ; )

#define PML4_ENTRY_BITS 39
#define PML4_ADDR(_slot) \
    (((_slot >> 8) * 0xffff000000000000) | (_slot << PML4_ENTRY_BITS))

    
