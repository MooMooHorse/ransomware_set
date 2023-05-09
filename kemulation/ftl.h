#ifndef __BYTEFS_FTL_H__
#define __BYTEFS_FTL_H__

// #include "ssd_nvme.h"
// #include <linux/printk.h>
#include <linux/printk.h>
#include <stdbool.h>
// #include <string.h>
#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/gfp.h>

// #include <time.h>
// #include <pthread.h>
// #include <thread.h>


#include <linux/kthread.h>  // for threads
#include <linux/sched.h>
#include <linux/time.h>   // for using jiffies 
#include <linux/timer.h>
#include <linux/timekeeping.h>
#include <linux/delay.h>
// #include <linux/bool.h>

#include <linux/kfifo.h>
#include <linux/circ_buf.h>


#include "timing_model.h"
#include "simple_nvme.h"
#include "ftl_mapping.h"
#include "ring.h"

// #include <linux/kthread.h>  // for threads
// #include <linux/time.h>   // for using jiffies 
// #include <linux/timer.h>

// #include "backend.c"

// // define of ssd parameters
// #define CH_COUNT        (8)
// #define WAY_COUNT       (4)
// // 4*8*1*128*128*16k  ----> 8GB

// changed to 2 G
#define CH_COUNT            (4)
#define WAY_COUNT           (2)
// 4*2*1*128*128*16k  ----> 2GB

// #define PL_COUNT 1
#define BLOCK_COUNT         (128)
#define PG_COUNT            (4 * 128)
#define PG_SIZE             (4 * 1024)
#if PG_SIZE % 1024 != 0
#error "Page size is not 1k aligned"
#endif
#define PG_MASK             (PG_SIZE - 1)

#define CACHE_SIZE          (2147483648UL)
#define TOTAL_SIZE          (2147483648UL)
#define ALL_TOTAL_SIZE      (4294967296ULL)

#define NUM_POLLERS         (1)
#define MAX_REQ             (65536)

#define LOG_DSIZE_GRANULARITY (128)


/* DRAM backend structure */
struct SsdDramBackend {
    void    *phy_loc;  
    void    *virt_loc;
    unsigned long size; /* in bytes */
};



/* nand request type */
#define NAND_READ   0
#define NAND_WRITE  1
#define NAND_ERASE  2


/* io type */
#define USER_IO     0
#define GC_IO       1
#define INTERNAL_TRANSFER 2

/* page status */
#define PG_FREE     0
#define PG_INVALID  1
#define PG_VALID    2


/* Page mapping defines */ 
// @TODO check it inited using this value
#define UNMAPPED_PPA (0xFFFFFFFF)   
#define INVALID_LPN (0xFFFFFFFF)


/* page index combination */
#define BLK_BITS    (16)
#define PG_BITS     (16)
// #define SEC_BITS    (8)
// #define PL_BITS     (8)
#define LUN_BITS    (8)
#define CH_BITS     (7)

/* describe a physical page addr -> real_ppa */
struct ppa {
    // union {
        struct {
            uint64_t blk : BLK_BITS;
            uint64_t pg  : PG_BITS;
            // uint64_t sec : SEC_BITS;
            // uint64_t pl  : PL_BITS;
            uint64_t lun : LUN_BITS;
            uint64_t ch  : CH_BITS;
            uint64_t rsv : 1;
        } g;
        uint64_t realppa;
    // };
};


/**
 * struct page - page structure
 * @pg_num: physical page number of the page -> real_ppa
 * @status: status of the page
 */
struct nand_page {  
    // struct ppa;
    int pg_num;
    int status;
};

/**
 * struct block - block structure
 * @pg: pages in this block )
 * @npgs: physical block number of the block
 * @vpc: valid page count of the block
 * @ipc: invalid page count of the block
 * @erase_cnt: times of the block being erased
 * @wp : write pointer -> speicificly which page are we going to write to
 */
struct nand_block {
    struct nand_page *pg;
    int npgs;
    int ipc; /* invalid page count */
    int vpc; /* valid page count */
    int erase_cnt;
    int wp; /* current write pointer */
};

/* plane is one so we just not include it at this version */
// struct nand_plane {
//     struct nand_block *blk;
//     int nblks;
// };

/**
 * struct lun - lun structure
 * @blk: blocks in this lun
 * @nblks: block count of the lun
 * @next_lun_avail_time: time for all request in this lun finishes
 * @busy: is lun working now? (not really used)
 */
struct nand_lun {
    struct nand_block *blk;
    int nblks;
    uint64_t next_lun_avail_time;
    bool busy;
    // uint64_t gc_endtime;
};


/**
 * struct channel - channel structure
 * @lun: luns in this channel
 * @nluns: lun count of the channel
 * @next_ch_avail_time: time for all request in this channel finishes
 * @busy: is channel working now? (not really used)
 */
struct ssd_channel {
    struct nand_lun *lun;
    int nluns;
    uint64_t next_ch_avail_time;
    bool busy;
    // uint64_t gc_endtime;
};

struct ssdparams {

    int pgsz;          /* page size in bytes */

    int pgs_per_blk;  /* # of NAND pages per block */
    int blks_per_lun;  /* # of blocks per plane */
    // int pls_per_lun;  /* # of planes per LUN (Die) */
    int luns_per_ch;  /* # of LUNs per channel */
    int nchs;         /* # of channels in the SSD */

    int pg_rd_lat;    /* NAND page read latency in nanoseconds */
    int pg_wr_lat;    /* NAND page program latency in nanoseconds */
    int blk_er_lat;   /* NAND block erase latency in nanoseconds */
    int ch_xfer_lat;  /* channel transfer latency for one page in nanoseconds
                       * this defines the channel bandwith
                       */

    // double gc_thres_pcent;
    // double gc_thres_pcent_high;
    // bool enable_gc_delay;

    /* below are all calculated values */
    // int pgs_per_lun;   /* # of pages per plane */
    int pgs_per_lun;  /* # of pages per LUN (Die) */
    int pgs_per_ch;   /* # of pages per channel */
    int tt_pgs;       /* total # of pages in the SSD */


    int blks_per_ch;  /* # of blocks per channel */
    int tt_blks;      /* total # of blocks in the SSD */

    int tt_luns;      /* total # of LUNs in the SSD */

    int num_poller;
};


/* wp: record next write page */
struct write_pointer {
    // struct line *curline;
    int ch;
    int lun;
    int blk;
    struct nand_block *blk_ptr;
    int pg;
};


struct nand_cmd {
    int type;
    int cmd;
    uint64_t stime; /* Coperd: request arrival time */ 
};


typedef enum {
    LOG_DATA, LOG_METADATA
} log_type;


struct log_info {
    uint64_t actual_lpa;
    log_type type;
    union {
        struct {
            void *data_start;
            unsigned long data_size;
        } data_info;
        struct {
            uint64_t reserved;
        } metadata_info;
    };
};


struct log_entry {
    uint64_t lpa;
    union {
        struct {
            uint8_t content[LOG_DSIZE_GRANULARITY];
        } data_entry;
        struct {
            uint64_t reserved;
        } metadata_entry;
    };
};


struct indirection_mte {
    unsigned long lpa;
    // ppa for flushed entries, log offset for in-log entries
    unsigned long addr;
    unsigned char flag;
};
// } __attribute__((packed));


struct ssd {
    char *ssdname;
    struct ssdparams sp;
    struct ssd_channel *ch;
    struct ppa *maptbl; /* page level mapping table */
    uint64_t *rmap;     /* reverse mapptbl, assume it's stored in OOB */
    struct write_pointer wp;
    // struct line_mgmt lm;

    /* lockless ring for communication with NVMe IO thread */
    struct Ring **to_ftl;
    struct Ring **to_poller;
    // bool *dataplane_started_ptr;

    //backend
    struct SsdDramBackend* bd;

    //thread
    struct task_struct *thread_id;
    struct task_struct *polling_thread_id;

    // mapping table
    struct indirection_mte *indirection_mt;
    int indirection_mt_size;
    // PPA array of flushed nand pages
    unsigned long *flushed_arr;
    unsigned flushed_arr_head;
    unsigned flushed_arr_tail;
    unsigned flushed_arr_size;

    // data buffers
    void *flush_wr_page_buf;
    void *flushed_page_buf;
    void *nand_page_buf;
    void *migration_dest_buf;
    void *migration_log_buf;
    
    // logs
    unsigned log_head;
    unsigned log_tail;
    unsigned log_size;
};




/** Backend.c */

extern struct SsdDramBackend *dram_backend;

int init_dram_space(void*phy_loc, void* virt_loc, unsigned int nbytes);

/* Memory Backend (mbe) for emulated SSD */

int init_dram_backend(struct SsdDramBackend **mbe, size_t nbytes, phys_addr_t phy_loc);


void free_dram_backend(struct SsdDramBackend *b);

// read or write to dram location
int backend_rw(struct SsdDramBackend *b, unsigned long ppa, void* data, bool is_write);

// read or write to dram location
int cache_rw(struct SsdDramBackend *b, unsigned long off, void* data, bool is_write,unsigned long size);

void *cache_mapped(struct SsdDramBackend *b, unsigned long off);


/** ftl.c */
extern struct ssd gdev;
extern uint64_t start, cur;






#define ftl_err(fmt, ...) \
    do { printk(KERN_ERR "[BYTEFS] FTL-Err: " fmt, ## __VA_ARGS__); } while (0)

// #define ftl_log(fmt, ...) \
//     do { printf("[BYTEFS] FTL-Log: " fmt, ## __VA_ARGS__); } while (0)

#define ftl_assert(x)                                                       \
do {    if (!(x))                                                    \
        printk(KERN_ERR "### ASSERTION FAILED %s: %s: %d: %s\n",      \
               __FILE__, __func__, __LINE__, #x); \
} while (0)


#define ftl_debug(x)                                                       \
do {                                                  \
        printk(KERN_ERR "### DEBUG %s: %s: %d: %s\n",      \
               __FILE__, __func__, __LINE__, #x); \
} while (0)

// /* BYTEFS assert() */
// #ifdef BYTEFS_DEBUG_FTL
// #define ftl_assert(expression) assert(expression)
// #else
// #define ftl_assert(expression)
// #endif

#endif
