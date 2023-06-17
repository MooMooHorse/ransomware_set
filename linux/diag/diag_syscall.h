#ifndef _DIAG_SYSCALL_H_
#define _DIAG_SYSCALL_H_

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/ktime.h>   
#include <linux/rbtree.h>
#include <linux/spinlock.h>
#include <linux/blk_types.h>
#include <linux/module.h>
#include <linux/backing-dev.h>
#include <linux/debugfs.h>
#include <linux/bpf.h>
#include <linux/printk.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>

#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/kernel_stat.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/completion.h>
#include <linux/slab.h>
#include <linux/swap.h>
#include <linux/writeback.h>
#include <linux/task_io_accounting_ops.h>
#include <linux/fault-inject.h>
#include <linux/list_sort.h>
#include <linux/delay.h>
#include <linux/ratelimit.h>
#include <linux/pm_runtime.h>
#include <linux/blk-cgroup.h>
#include <linux/debugfs.h>
#include <linux/bpf.h>
#include <linux/printk.h>

// #define MAX_NUM_TRACE       (1 << 27) // 128 M traces
#define MAX_DISKS           50

// after 16 contiguous magic number (wihtin a byte), we mark this block as uneencrypted     
// after 16 contiguous encrypted magic number, we mark this block as encrypted
#define DYE_THRESHOLD       16 



typedef enum IO_TYPE {
    IO_READ = 0,
    IO_WRITE
} io_type_t;

typedef struct diag_ctrl_entry {
    uint64_t time;
    io_type_t IO_type;
    uint64_t lpa; // although it's called lpa, it's in sector unit (lun size is 512 bytes) TO FIX
    int len; // in sector unit
    int unencrypted_len,  encrypted_len; // in sector unit
} diag_ctrl_entry_t;

typedef struct diag_ctrl {
    // inputs 

    // int dump_real_trace;
    
    uint64_t magic;
    // backing device
    uint64_t num_disk;
    char* monitored_disk[MAX_DISKS];
    // output

    // trace buffer
    // uint64_t num_trace;
    // diag_ctrl_entry_t* trace;

    uint64_t trace_status; // on/off
    uint64_t* blk2file;
    uint64_t blk2file_size;
} diag_ctrl_t;

typedef struct CACHE_ENTRY {
    uint64_t lsa;
    uint32_t is_encrypted;
    struct rb_node node;
} cache_entry_t;

typedef struct BIO_cache {
    uint64_t unencrypted_len, encrypted_len; // # blocks of unencrypted/encrypted data
    int min_len;
    struct rb_root cache_root;
    int rans_enabled;
    int trace_status;
    spinlock_t lock;
} bio_cache_t;

// diag_ctrl_entry_t* get_trace(); // get trace buffer

// uint64_t get_num_trace(); // get number of trace

// diag_ctrl_entry_t* alloc_trace(); // allocate trace buffer

int set_disks(uint64_t num_disks, char** disks); // set disks to monitor

void turn_on_trace(void); // turn on trace

void turn_off_trace(void); // turn off trace

int is_trace_on(void); // check if trace is on

int get_encrpted(const char* buf, uint64_t sec_off); // check if a block is encrypted

int get_unencrypted(const char* buf, uint64_t sec_off); // check if a block is unencrypted

// update bio_cache
int64_t rb_update(struct rb_root* root, uint64_t lpa, struct rb_node** node, uint64_t is_encrypted); 

void clear_all(void); // clear trace and cache

void delete_rb_tree(struct rb_root* root); // delete rb tree

// int add_trace(uint64_t time, io_type_t IO_type, uint64_t lba, uint64_t len, 
//  uint64_t unencrypted_len, uint64_t encrypted_len); // add a trace to trace buffer

void turn_on_rans(void); // turn on ransomware

void turn_off_rans(void); // turn off ransomware

uint64_t get_blk2file_size(void); // get blk2file size

void get_blk2file(uint64_t* __user buf); // get blk2file to user space

int diag_proc_bio(struct bio* bio); // process bio

void set_magic(uint64_t magic); // set magic number

#endif /* _DIAG_SYSCALL_H_ */