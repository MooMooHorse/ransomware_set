#ifndef _SYS_H_
#define _SYS_H_

#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#define MAX_DISKS           50

typedef enum OPT {
    HINT = 0,
    START_TRACE, // start recording blk2file map
    END_TRACE, // stop recording blk2file map
    CLR_TRACE, // clear blk2file map
    GET_MAP, // get blk2file map
    SET_MAGIC // set magic number
} opt_t;

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

int call(int opt, diag_ctrl_t* ctrl);

#endif /* _SYS_H_ */
