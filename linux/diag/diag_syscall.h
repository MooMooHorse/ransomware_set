#ifndef _DIAG_SYSCALL_H_
#define _DIAG_SYSCALL_H_

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/ktime.h>   

#define MAX_NUM_TRACE 10000000
#define MAX_DISKS     50



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
    // trace buffer
    uint64_t num_trace;
    diag_ctrl_entry_t* trace;
    // backing device
    uint64_t num_disk;
    char* monitored_disk[MAX_DISKS];
} diag_ctrl_t;

diag_ctrl_entry_t* get_trace(); // get trace buffer

uint64_t get_num_trace(); // get number of trace

diag_ctrl_entry_t* alloc_trace(); // allocate trace buffer

int set_disks(uint64_t num_disks, char** disks); // set disks to monitor




#endif /* _DIAG_SYSCALL_H_ */