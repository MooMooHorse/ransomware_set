#ifndef _DIAG_H_
#define _DIAG_H_

#include <linux/kernel.h>
#include <linux/sched/signal.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/iomap.h>
#include <linux/mm.h>
#include <linux/percpu.h>
#include <linux/slab.h>
#include <linux/capability.h>
#include <linux/blkdev.h>
#include <linux/file.h>
#include <linux/quotaops.h>
#include <linux/highmem.h>
#include <linux/export.h>
#include <linux/backing-dev.h>
#include <linux/writeback.h>
#include <linux/hash.h>
#include <linux/suspend.h>
#include <linux/buffer_head.h>
#include <linux/task_io_accounting_ops.h>
#include <linux/bio.h>
#include <linux/cpu.h>
#include <linux/bitops.h>
#include <linux/mpage.h>
#include <linux/bit_spinlock.h>
#include <linux/pagevec.h>
#include <linux/sched/mm.h>


#define TAR_FILE_MAX                1          // maximum number of files to monitor

#define DIAG_EN                     1          // enable diag hint

typedef struct CONFIG {
    char* target_files[TAR_FILE_MAX];           // file names to monitor
    uint64_t ino[TAR_FILE_MAX];          // # inode
} config_t;

int diag_block_write_begin(struct file *file, struct page *page, loff_t pos, unsigned len,
				  get_block_t *get_block);

int DIAG_INODE_IS_TAR(struct inode* inode);     // check if inode is in target_files 
int DIAG_FILE_IS_TAR(struct file* file);        // check if file is in target_files
int dumpNcache_file(struct file* file, struct inode *inode);  // check if file is in target_files and cache its inode number

void parse_kiocb_flags(struct file* file);     // parse kiocb flags

#define DIAG_HINT(if_print, loglevel, fmt, args...) \
    if(if_print) printk(loglevel fmt, ##args)

#endif /* _DIAG_H_ */