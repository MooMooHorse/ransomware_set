#ifndef _DIAG_SYSCALL_H_
#define _DIAG_SYSCALL_H_

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/ktime.h>   
#include <linux/rbtree.h>
#include <linux/spinlock.h>

#define MAX_NUM_TRACE       10000000
#define MAX_DISKS           50

// after 16 contiguous magic number (wihtin a byte), we mark this block as uneencrypted     
// after 16 contiguous encrypted magic number, we mark this block as encrypted
#define DYE_THRESHOLD       16 

typedef enum ENCODING_TYPE {
    UTF8 = 0,
    UTF16,
} encoding_t;


typedef enum IO_TYPE {
    IO_READ = 0,
    IO_WRITE
} io_type_t;

typedef struct diag_ctrl_entry {
    uint64_t time;
    io_type_t IO_type;
    uint64_t lpa;
    uint64_t len;
} diag_ctrl_entry_t;

typedef struct diag_ctrl {
    // inputs 

    int dump_real_trace;

    // output

    // trace buffer
    uint64_t num_trace;
    diag_ctrl_entry_t* trace;
    // backing device
    uint64_t num_disk;
    uint64_t trace_status; // on/off
    char* monitored_disk[MAX_DISKS];
} diag_ctrl_t;

typedef struct CACHE_ENTRY {
    uint64_t lpa;
    uint32_t is_encrypted;
    struct rb_node node;
} cache_entry_t;

typedef struct BIO_cache {
    uint64_t unencrypted_len, encrypted_len; // # blocks of unencrypted/encrypted data
    uint64_t magic;
    int min_len;
    struct rb_root cache_root;
    encoding_t encoding;
    spinlock_t lock;
} bio_cache_t;

diag_ctrl_entry_t* get_trace(); // get trace buffer

uint64_t get_num_trace(); // get number of trace

diag_ctrl_entry_t* alloc_trace(); // allocate trace buffer

int set_disks(uint64_t num_disks, char** disks); // set disks to monitor

void turn_on_trace(); // turn on trace

void turn_off_trace(); // turn off trace

int is_trace_on(); // check if trace is on

int get_encrpted(const char* buf, uint64_t sec_off); // check if a block is encrypted

int get_unencrypted(const char* buf, uint64_t sec_off); // check if a block is unencrypted

// update bio_cache
int64_t rb_update(struct rb_root* root, uint64_t lpa, struct rb_node** node, uint64_t is_encrypted); 

void clear_all(); // clear trace and cache

#endif /* _DIAG_SYSCALL_H_ */