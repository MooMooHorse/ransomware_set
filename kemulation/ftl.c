#include "ftl.h"
#include "bytefs.h"
#include "backend.h"
#include "indirection_mt.h"

#include <linux/hrtimer.h>

// #include <unistd.h>

struct ssd gdev;

// static struct hrtimer ftl_timer;

uint64_t start, cur;

// this is a naive initialization of the nvme cmd, we just put addr at prp1
void bytefs_init_nvme(NvmeCmd* req, int op, uint64_t lba, uint32_t nlb, void* addr){
    req->opcode = op;
    req->fuse = 0;
    req->psdt = 0;
    req->cid = 0;
    req->nsid = 1;
    req->mptr = 0;
    req->dptr.prp1 = (uint64_t)addr;
    req->dptr.prp2 = 0;
    req->cdw10 = lba;
    req->cdw11 = 0;
    req->cdw12 = nlb;
    req->cdw13 = 0;
    req->cdw14 = 0;
    req->cdw15 = 0;
}



static uint64_t timespec2uint(struct timespec64 *t)
{
    return (uint64_t)t->tv_sec * 1000000000UL + t->tv_nsec;
}

static uint64_t uint2timespec(uint64_t time, struct timespec64 *t)
{
    t->tv_nsec = time % 1000000000UL;
    t->tv_sec = (time - t->tv_nsec) / 1000000000UL;
    // *t = (s64)time;
}


static uint64_t get_time_ns(void)
{
    struct timespec64 ts;
    ktime_get_ts64(&ts);
    return timespec2uint(&ts);
}


static void sleepns(uint64_t ns) {
    struct timespec64 abs_wait_barrier;

    // ktime_get_ts64(&abs_wait_barrier);
    uint2timespec( ns, &abs_wait_barrier);
    // clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &abs_wait_barrier, NULL);
    long ret = hrtimer_nanosleep(&abs_wait_barrier, HRTIMER_MODE_REL, CLOCK_MONOTONIC);

    //@TODO check this
}

/**
 * get the physical address of the page
 * @param lpn: logical page number
 * @param ssd: ssd structure
 * @return: physical address of the page by struct ppa
 */
static inline struct ppa get_maptbl_ent(struct ssd *ssd, uint64_t lpn)
{

    ftl_assert(lpn < ssd->sp.tt_pgs); //@TODO make this number less if add gc
    return ssd->maptbl[lpn];
}

/**
 * set the physical address of the page
 */
static inline void set_maptbl_ent(struct ssd *ssd, uint64_t lpn, struct ppa *ppa)
{
    ftl_assert(lpn < ssd->sp.tt_pgs);
    ssd->maptbl[lpn] = *ppa;
}

/**
 * get the realppa (physical page index) of the page
 * 
 */
static uint64_t ppa2pgidx(struct ssd *ssd, struct ppa *ppa)
{
    struct ssdparams *spp = &ssd->sp;
    uint64_t pgidx;

    pgidx = ppa->g.ch  * spp->pgs_per_ch  + \
            ppa->g.lun * spp->pgs_per_lun + \
            ppa->g.blk * spp->pgs_per_blk + \
            ppa->g.pg;

    ftl_assert(pgidx < spp->tt_pgs);
    return pgidx;
}

static void pgidx2ppa(struct ssd *ssd, struct ppa *ppa)
{
    struct ssdparams *spp = &ssd->sp;
    uint64_t pgidx = ppa->realppa;
    ftl_assert(pgidx < spp->tt_pgs);

    ppa->g.ch = pgidx / spp->pgs_per_ch;
    pgidx %= spp->pgs_per_ch;
    ppa->g.lun = pgidx / spp->pgs_per_lun;
    pgidx %= spp->pgs_per_lun;
    ppa->g.blk = pgidx / spp->pgs_per_blk;
    pgidx %= spp->pgs_per_blk;
    ppa->g.pg = pgidx;
}

static inline uint64_t get_rmap_ent(struct ssd *ssd, struct ppa *ppa)
{
    uint64_t pgidx = ppa2pgidx(ssd, ppa);

    return ssd->rmap[pgidx];
}

/* set rmap[page_no(ppa)] -> lpn */
static inline void set_rmap_ent(struct ssd *ssd, uint64_t lpn, struct ppa *ppa)
{
    uint64_t pgidx = ppa2pgidx(ssd, ppa);

    ssd->rmap[pgidx] = lpn;
}


static void ssd_init_write_pointer(struct ssd *ssd)
{
    struct write_pointer *wpp = &ssd->wp;
    wpp->ch = 0;
    wpp->lun = 0;
    wpp->pg = 0;
    wpp->blk = 0;
    wpp->blk_ptr = &(ssd->ch[wpp->ch].lun[wpp->lun].blk[wpp->blk]);

}

static inline void check_addr(int a, int max)
{
    ftl_assert(a >= 0 && a < max);
}


// static void ssd_advance_write_pointer(struct ssd *ssd)
// {
//     struct ssdparams *spp = &ssd->sp;
//     struct write_pointer *wpp = &ssd->wp;
//     // struct line_mgmt *lm = &ssd->lm;

//     check_addr(wpp->ch, spp->nchs);
//     wpp->ch++;
//     if (wpp->ch == spp->nchs) {
//         wpp->ch = 0;
//         check_addr(wpp->lun, spp->luns_per_ch);
//         wpp->lun++;
//         /* in this case, we should go to next lun */
//         if (wpp->lun == spp->luns_per_ch) {
//             wpp->lun = 0;
//             /* go to next page in the block */
//             check_addr(wpp->pg, spp->pgs_per_blk);
//             wpp->pg++;
//             if (wpp->pg == spp->pgs_per_blk) {
//                 wpp->pg = 0;
//                 /* move current block to {victim,full} block list */
//                 if (wpp->blk_ptr->vpc == spp->pgs_per_line) {
//                     /* all pgs are still valid, move to full line list */
//                     ftl_assert(wpp->curline->ipc == 0);
//                     QTAILQ_INSERT_TAIL(&lm->full_line_list, wpp->curline, entry);
//                     lm->full_line_cnt++;
//                 } else {
//                     ftl_assert(wpp->curline->vpc >= 0 && wpp->curline->vpc < spp->pgs_per_line);
//                     /* there must be some invalid pages in this line */
//                     ftl_assert(wpp->curline->ipc > 0);
//                     pqueue_insert(lm->victim_line_pq, wpp->curline);
//                     lm->victim_line_cnt++;
//                 }
//                 /* current block is used up, pick another empty line */
//                 check_addr(wpp->blk, spp->blks_per_pl);
//                 wpp->curline = NULL;
//                 wpp->curline = get_next_free_line(ssd);
//                 if (!wpp->curline) {
//                     /* TODO */
//                     abort();
//                 }
//                 wpp->blk = wpp->curline->id;
//                 check_addr(wpp->blk, spp->blks_per_lun);
//                 /* make sure we are starting from page 0 in the super block */ 
//                 /* why we check super block here? Ok so line is superblock? 
//                 *  So here for bytefs we translate at page granularity and write at block granularity (gather in buffer)
//                 * */
//                 // ftl_assert(wpp->pg == 0);
//                 // ftl_assert(wpp->lun == 0);
//                 // ftl_assert(wpp->ch == 0);
//                 /* TODO: assume # of pl_per_lun is 1, fix later */
//                 // ftl_assert(wpp->pl == 0);
//             }
//         }
//     }
// }



// rewrite advance write pointer to block granularity
static void ssd_advance_write_pointer(struct ssd *ssd)
{
    struct ssdparams *spp = &ssd->sp;
    struct write_pointer *wpp = &ssd->wp;
    // struct line_mgmt *lm = &ssd->lm;

    // static int req_cnt = 0;
    // req_cnt++;
    // printf("REQ CNT %d\n", req_cnt);

    check_addr(wpp->ch, spp->nchs);
    wpp->pg++;
    // first check if we reached last page in block:
    if (wpp->pg == spp->pgs_per_blk ) {
        // add block to full list
        //@TODO


        // if so, move to next block
        wpp->pg = 0;
        // move previous blk's wp to pg_per_blk
        wpp->blk_ptr->wp = spp->pgs_per_blk;

        //first channel
        check_addr(wpp->lun, spp->luns_per_ch);
        wpp->ch++;
        if (wpp->ch == spp->nchs) {
            wpp->ch = 0;
            check_addr(wpp->lun, spp->luns_per_ch);
            wpp->lun++;
            if (wpp->lun == spp->luns_per_ch) {
                wpp->lun = 0;
                wpp->blk++;
                ftl_assert(wpp->blk <= spp->blks_per_lun);
            }
        }
        wpp->blk_ptr = &(ssd->ch[wpp->ch].lun[wpp->lun].blk[wpp->blk]);
    } else {
        // if not, move to next page
        // wpp->pg++;
        wpp->blk_ptr->wp++;
    }
    // printf("wpp->blk_ptr->wp: %d, wpp->ch:%d, wpp->lun:%d, wpp->blk:%d, wpp->pg:%d", wpp->blk_ptr->wp, wpp->ch, wpp->lun, wpp->blk, wpp->pg);


    ftl_assert(wpp->pg != spp->pgs_per_blk);
    ftl_assert(wpp->lun != spp->luns_per_ch);
    ftl_assert(wpp->ch != spp->nchs);

    ftl_assert(wpp->blk != spp->blks_per_lun);
    ftl_assert(wpp->blk_ptr->wp == wpp->pg);
    // ftl_assert(wpp->blk_ptr == &(ssd->ch[wpp->ch].lun[wpp->lun].blk[wpp->blk]));
}


static struct ppa get_new_page(struct ssd *ssd)
{
    struct write_pointer *wpp = &ssd->wp;
    struct ppa ppa;
    ppa.realppa = 0;
    ppa.g.ch = wpp->ch;
    ppa.g.lun = wpp->lun;
    ppa.g.pg = wpp->pg;
    ppa.g.blk = wpp->blk;
    // ppa.g.pl = wpp->pl;
    // ftl_assert(ppa.g.pl == 0);
    ppa.realppa = ppa2pgidx(ssd, &ppa);
    return ppa;
}

static void check_params(struct ssdparams *spp)
{
    /*
     * we are using a general write pointer increment method now, no need to
     * force luns_per_ch and nchs to be power of 2
     */

//     ftl_assert(is_power_of_2(spp->luns_per_ch));
//     ftl_assert(is_power_of_2(spp->nchs));
}

static void ssd_init_params(struct ssdparams *spp)
{

    spp->pgsz = PG_SIZE;

    spp->pgs_per_blk = PG_COUNT;
    spp->blks_per_lun = BLOCK_COUNT;
    spp->luns_per_ch = WAY_COUNT;
    spp->nchs = CH_COUNT;

    spp->pg_rd_lat = NAND_READ_LATENCY;
    spp->pg_wr_lat = NAND_PROG_LATENCY;    
    spp->blk_er_lat = NAND_BLOCK_ERASE_LATENCY;   
    spp->ch_xfer_lat = CHNL_TRANSFER_LATENCY_NS; 

    spp->pgs_per_lun = spp->pgs_per_blk * spp->blks_per_lun;
    spp->pgs_per_ch = spp->pgs_per_lun * spp->luns_per_ch;   
    spp->tt_pgs = spp-> pgs_per_ch * spp->nchs;


    spp->blks_per_ch = spp->blks_per_lun * spp->luns_per_ch;  
    spp->tt_blks = spp->blks_per_ch * spp->nchs;     
    spp->tt_luns = spp->luns_per_ch * spp->nchs;      

    spp->num_poller = NUM_POLLERS;

    // GC related
    // spp->gc_thres_pcent = 0.75;
    // spp->gc_thres_lines = (int)((1 - spp->gc_thres_pcent) * spp->tt_lines);
    // spp->gc_thres_pcent_high = 0.95;
    // spp->gc_thres_lines_high = (int)((1 - spp->gc_thres_pcent_high) * spp->tt_lines);
    // spp->enable_gc_delay = true;

    check_params(spp);
}

// merge this function into the blk init
// static void ssd_init_nand_page(struct nand_page *pg, struct ssdparams *spp)
// {
//     pg->pg_num = ;
//     pg->sec = malloc(sizeof(nand_sec_status_t) * pg->nsecs);
//     for (int i = 0; i < pg->nsecs; i++) {
//         pg->sec[i] = SEC_FREE;
//     }
//     pg->status = PG_FREE;
// }

static void ssd_init_nand_blk(struct nand_block *blk, struct ssdparams *spp)
{
    int i =0;
    blk->npgs = spp->pgs_per_blk;
    blk->pg = kzalloc(sizeof(struct nand_page) * blk->npgs,GFP_KERNEL);
    for (i = 0; i < blk->npgs; i++) {
        blk->pg[i].pg_num = i;
        blk->pg[i].status = PG_FREE;
    }
    blk->ipc = 0;
    // vpc here should be inited to 0
    // blk->vpc = spp->pgs_per_blk;
    blk->vpc = 0;
    blk->erase_cnt = 0;
    blk->wp = 0;
}

// we only has one plane here add this if needed in future
// static void ssd_init_nand_plane(struct nand_plane *pl, struct ssdparams *spp)
// {
//     pl->nblks = spp->blks_per_pl;
//     pl->blk = malloc(sizeof(struct nand_block) * pl->nblks);
//     for (int i = 0; i < pl->nblks; i++) {
//         ssd_init_nand_blk(&pl->blk[i], spp);
//     }
// }

static void ssd_init_nand_lun(struct nand_lun *lun, struct ssdparams *spp)
{
    int i;
    lun->nblks = spp->blks_per_lun;
    lun->blk = kzalloc(sizeof(struct nand_block) * lun->nblks,GFP_KERNEL);
    for (i = 0; i < lun->nblks; i++) {
        ssd_init_nand_blk(&lun->blk[i], spp);
    }
    lun->next_lun_avail_time = 0;
    lun->busy = false;
}

static void ssd_init_ch(struct ssd_channel *ch, struct ssdparams *spp)
{
    int i;
    ch->nluns = spp->luns_per_ch;
    ch->lun = kzalloc(sizeof(struct nand_lun) * ch->nluns,GFP_KERNEL);
    for (i = 0; i < ch->nluns; i++) {
        ssd_init_nand_lun(&ch->lun[i], spp);
    }
    ch->next_ch_avail_time = 0;
    ch->busy = 0;
}

static void ssd_init_maptbl(struct ssd *ssd)
{
    int i;
    struct ssdparams *spp = &ssd->sp;
    ssd->maptbl = kzalloc(sizeof(struct ppa) * spp->tt_pgs,GFP_KERNEL);
    for (i = 0; i < spp->tt_pgs; i++) {
        ssd->maptbl[i].realppa = UNMAPPED_PPA;
    }
}

static void ssd_init_rmap(struct ssd *ssd)
{
    int i;
    struct ssdparams *spp = &ssd->sp;
    ssd->rmap = kzalloc(sizeof(uint64_t) * spp->tt_pgs,GFP_KERNEL);
    for (i = 0; i < spp->tt_pgs; i++) {
        ssd->rmap[i] = INVALID_LPN;
    }
}



static void ssd_init_queues(struct ssd *ssd)
{
    int i;
    struct ssdparams *spp = &ssd->sp;

    ssd->to_poller = kcalloc(spp->num_poller+1,sizeof(void*),GFP_KERNEL);
    ssd->to_ftl = kcalloc(spp->num_poller+1,sizeof(void*),GFP_KERNEL);

    //@TODO check i = 1 here !!!
    for (i=1; i <= spp->num_poller; i++) {
        ssd->to_poller[i] = ring_alloc(MAX_REQ,1);
        ssd->to_ftl[i] = ring_alloc(MAX_REQ,1);
    }
    
}



void ssd_end(void){
    struct ssd *ssd = &gdev;
    // pthread_join(ssd->thread_id, NULL);

}

static inline bool valid_ppa(struct ssd *ssd, struct ppa *ppa)
{
    struct ssdparams *spp = &ssd->sp;
    int ch = ppa->g.ch;
    int lun = ppa->g.lun;
    // int pl = ppa->g.pl;
    int blk = ppa->g.blk;
    int pg = ppa->g.pg;
    // int sec = ppa->g.sec;

    if ( ch >= 0 && ch < spp->nchs 
        && lun >= 0 && lun < spp->luns_per_ch 
        // && pl >=   0 && pl < spp->pls_per_lun 
        && blk >= 0 && blk < spp->blks_per_lun 
        && pg >= 0 && pg < spp->pgs_per_blk 
        // && sec >= 0 && sec < spp->secs_per_pg 
         )
        return true;

    return false;
}

// maybe changed according to GC?
static inline bool valid_lpn(struct ssd *ssd, uint64_t lpn)
{
    return (lpn < ssd->sp.tt_pgs);
}

static inline bool mapped_ppa(struct ppa *ppa)
{
    return !(ppa->realppa == UNMAPPED_PPA);
}

static inline struct ssd_channel *get_ch(struct ssd *ssd, struct ppa *ppa)
{
    return &(ssd->ch[ppa->g.ch]);
}

static inline struct nand_lun *get_lun(struct ssd *ssd, struct ppa *ppa)
{
    struct ssd_channel *ch = get_ch(ssd, ppa);
    return &(ch->lun[ppa->g.lun]);
}

// static inline struct nand_plane *get_pl(struct ssd *ssd, struct ppa *ppa)
// {
//     struct nand_lun *lun = get_lun(ssd, ppa);
//     return &(lun->pl[ppa->g.pl]);
// }

static inline struct nand_block *get_blk(struct ssd *ssd, struct ppa *ppa)
{
    // struct nand_plane *pl = get_pl(ssd, ppa);
    struct nand_lun *lun = get_lun(ssd, ppa);
    return &(lun->blk[ppa->g.blk]);
}

// static inline struct line *get_line(struct ssd *ssd, struct ppa *ppa)
// {
//     return &(ssd->lm.lines[ppa->g.blk]);
// }                        ,,,,,,,,,,,,,,,,,,,,,

static inline struct nand_page *get_pg(struct ssd *ssd, struct ppa *ppa)
{
    struct nand_block *blk = get_blk(ssd, ppa);
    return &(blk->pg[ppa->g.pg]);
}



/** key function that calculates latency */
static uint64_t ssd_advance_status(struct ssd *ssd, struct ppa *ppa, struct
        nand_cmd *ncmd)
{
    int c = ncmd->cmd;
    cur = get_time_ns();
    uint64_t cmd_stime = (ncmd->stime == 0) ? \
        cur :  ncmd->stime;
    uint64_t nand_stime;
    struct ssdparams *spp = &ssd->sp;
    struct nand_lun *lun = get_lun(ssd, ppa);
    uint64_t lat = 0;

    switch (c) {
    case NAND_READ:
        /* read: perform NAND cmd first */
        nand_stime = (lun->next_lun_avail_time < cmd_stime) ? cmd_stime : \
                     lun->next_lun_avail_time;
        lun->next_lun_avail_time = nand_stime + spp->pg_rd_lat;
        lat = lun->next_lun_avail_time - cmd_stime;

        break;

    case NAND_WRITE:
        /* write: transfer data through channel first */
        nand_stime = (lun->next_lun_avail_time < cmd_stime) ? cmd_stime : \
                     lun->next_lun_avail_time;
        if (ncmd->type == USER_IO) {
            lun->next_lun_avail_time = nand_stime + spp->pg_wr_lat;
        } else {
            lun->next_lun_avail_time = nand_stime + spp->pg_wr_lat;
        }
        lat = lun->next_lun_avail_time - cmd_stime;


        break;

    case NAND_ERASE:
        /* erase: only need to advance NAND status */
        nand_stime = (lun->next_lun_avail_time < cmd_stime) ? cmd_stime : \
                     lun->next_lun_avail_time;
        lun->next_lun_avail_time = nand_stime + spp->blk_er_lat;

        lat = lun->next_lun_avail_time - cmd_stime;
        break;

    default:
        ftl_err("Unsupported NAND command: 0x%x\n", c);
    }

    return lat;
}

/* update SSD status about one page from PG_VALID -> PG_VALID */
static void mark_page_invalid(struct ssd *ssd, struct ppa *ppa)
{
    // printf("PPA MARK INVALID %lX\n", ppa->realppa);
    struct ssdparams *spp = &ssd->sp;
    struct nand_block *blk = NULL;
    struct nand_page *pg = NULL;


    /* update corresponding page status */
    pg = get_pg(ssd, ppa);
    ftl_assert(pg->status == PG_VALID);
    pg->status = PG_INVALID;

    /* update corresponding block status */
    blk = get_blk(ssd, ppa);
    ftl_assert(blk->ipc >= 0 && blk->ipc < spp->pgs_per_blk);
    blk->ipc++;
    ftl_assert(blk->vpc > 0 && blk->vpc <= spp->pgs_per_blk);
    blk->vpc--;


}


/* mark current page as valid */
static void mark_page_valid(struct ssd *ssd, struct ppa *ppa)
{
    // printf("PPA MARK VALID %lX\n", ppa->realppa);
    struct nand_block *blk = NULL;
    struct nand_page *pg = NULL;
    // struct line *line;

    /* update page status */
    pg = get_pg(ssd, ppa);
    ftl_assert(pg->status == PG_FREE);
    pg->status = PG_VALID;

    /* update corresponding block status */
    blk = get_blk(ssd, ppa);
    ftl_assert(blk->vpc >= 0 || blk->vpc < ssd->sp.pgs_per_blk);
    blk->vpc++;

}


static void mark_block_free(struct ssd *ssd, struct ppa *ppa)
{
    struct ssdparams *spp = &ssd->sp;
    struct nand_block *blk = get_blk(ssd, ppa);
    struct nand_page *pg = NULL;
    int i;
    for (i = 0; i < spp->pgs_per_blk; i++) {
        /* reset page status */
        pg = &blk->pg[i];
        // ftl_assert(pg->nsecs == spp->secs_per_pg);
        pg->status = PG_FREE;
    }

    /* reset block status */
    ftl_assert(blk->npgs == spp->pgs_per_blk);
    blk->ipc = 0;
    blk->vpc = 0;
    blk->erase_cnt++;
}

// static void gc_read_page(struct ssd *ssd, struct ppa *ppa)
// {
//     /* advance ssd status, we don't care about how long it takes */
//     if (ssd->sp.enable_gc_delay) {
//         struct nand_cmd gcr;
//         gcr.type = GC_IO;
//         gcr.cmd = NAND_READ;
//         gcr.stime = 0;
//         ssd_advance_status(ssd, ppa, &gcr);
//     }
// }

// /* move valid page data (already in DRAM) from victim line to a new page */
// static uint64_t gc_write_page(struct ssd *ssd, struct ppa *old_ppa)
// {
//     struct ppa new_ppa;
//     struct nand_lun *new_lun;
//     uint64_t lpn = get_rmap_ent(ssd, old_ppa);

//     ftl_assert(valid_lpn(ssd, lpn));
//     new_ppa = get_new_page(ssd);
//     /* update maptbl */
//     set_maptbl_ent(ssd, lpn, &new_ppa);
//     /* update rmap */
//     set_rmap_ent(ssd, lpn, &new_ppa);

//     mark_page_valid(ssd, &new_ppa);

//     /* need to advance the write pointer here */
//     ssd_advance_write_pointer(ssd);

//     if (ssd->sp.enable_gc_delay) {
//         struct nand_cmd gcw;
//         gcw.type = GC_IO;
//         gcw.cmd = NAND_WRITE;
//         gcw.stime = 0;
//         ssd_advance_status(ssd, &new_ppa, &gcw);
//     }

//     /* advance per-ch gc_endtime as well */
// #if 0
//     new_ch = get_ch(ssd, &new_ppa);
//     new_ch->gc_endtime = new_ch->next_ch_avail_time;
// #endif

//     new_lun = get_lun(ssd, &new_ppa);
//     new_lun->gc_endtime = new_lun->next_lun_avail_time;

//     return 0;
// }



/* here ppa identifies the block we want to clean */
// static void clean_one_block(struct ssd *ssd, struct ppa *ppa)
// {
//     struct ssdparams *spp = &ssd->sp;
//     struct nand_page *pg_iter = NULL;
//     int cnt = 0;

//     for (int pg = 0; pg < spp->pgs_per_blk; pg++) {
//         ppa->g.pg = pg;
//         pg_iter = get_pg(ssd, ppa);
//         /* there shouldn't be any free page in victim blocks */
//         ftl_assert(pg_iter->status != PG_FREE);
//         if (pg_iter->status == PG_VALID) {
//             gc_read_page(ssd, ppa);
//             /* delay the maptbl update until "write" happens */
//             gc_write_page(ssd, ppa);
//             cnt++;
//         }
//     }

//     ftl_assert(get_blk(ssd, ppa)->vpc == cnt);
// }



// static int do_gc(struct ssd *ssd, bool force)
// {
//     struct line *victim_line = NULL;
//     struct ssdparams *spp = &ssd->sp;
//     struct nand_lun *lunp;
//     struct ppa ppa;
//     int ch, lun;

//     victim_line = select_victim_line(ssd, force);
//     if (!victim_line) {
//         return -1;
//     }

//     ppa.g.blk = victim_line->id;
//     ftl_debug("GC-ing line:%d,ipc=%d,victim=%d,full=%d,free=%d\n", ppa.g.blk,
//               victim_line->ipc, ssd->lm.victim_line_cnt, ssd->lm.full_line_cnt,
//               ssd->lm.free_line_cnt);

//     /* copy back valid data */
//     for (ch = 0; ch < spp->nchs; ch++) {
//         for (lun = 0; lun < spp->luns_per_ch; lun++) {
//             ppa.g.ch = ch;
//             ppa.g.lun = lun;
//             ppa.g.pl = 0;
//             lunp = get_lun(ssd, &ppa);
//             clean_one_block(ssd, &ppa);
//             mark_block_free(ssd, &ppa);

//             if (spp->enable_gc_delay) {
//                 struct nand_cmd gce;
//                 gce.type = GC_IO;
//                 gce.cmd = NAND_ERASE;
//                 gce.stime = 0;
//                 ssd_advance_status(ssd, &ppa, &gce);
//             }

//             lunp->gc_endtime = lunp->next_lun_avail_time;
//         }
//     }

//     /* update line status */
//     mark_line_free(ssd, &ppa);

//     return 0;
// }

static void advance_log_head(struct ssd *ssd) {
    ssd->log_head++;
    ssd->log_size--;
    ftl_assert(ssd->log_size >= 0);
    if (ssd->log_head == LOG_ENTRIES) ssd->log_head = 0;
}

static void advance_log_tail(struct ssd *ssd) {
    ssd->log_tail++;
    ssd->log_size++;
    ftl_assert(ssd->log_size <= LOG_ENTRIES);
    if (ssd->log_tail == LOG_ENTRIES) ssd->log_tail = 0;
}

static void advance_flushed_arr_head(struct ssd *ssd) {
    ssd->flushed_arr_head++;
    ssd->flushed_arr_size--;
    ftl_assert(ssd->flushed_arr_size >= 0);
    if (ssd->flushed_arr_head == FLUSHED_ARRAY_ENTRIES) ssd->flushed_arr_head = 0;
}

static void advance_flushed_arr_tail(struct ssd *ssd) {
    ssd->flushed_arr_tail++;
    ssd->flushed_arr_size++;
    ftl_assert(ssd->flushed_arr_size <= FLUSHED_ARRAY_ENTRIES);
    if (ssd->flushed_arr_tail == FLUSHED_ARRAY_ENTRIES) ssd->flushed_arr_tail = 0;
}

static void internal_transfer_metadata_handle(struct ssd *ssd, unsigned long ppa, int is_write) {
    struct nand_cmd ncmd;
    uint64_t start_time;
    struct ppa ppa_struct;
    // clock_gettime(CLOCK_MONOTONIC, &start_time);
    start_time = get_time_ns();

    ncmd.type = INTERNAL_TRANSFER;
    if (is_write) {
        ncmd.cmd = NAND_WRITE;
    } else {
        ncmd.cmd = NAND_READ;
    }
    ncmd.stime = start_time;

    ppa_struct.realppa = ppa;
    ssd_advance_status(ssd, &ppa_struct, &ncmd);
}

static void read_data(struct ssd *ssd, unsigned long start_lpa, 
                      unsigned long size, void *data) {
    ftl_assert(size % LOG_DSIZE_GRANULARITY == 0);
    // last_*_ppa_pg_aligned is set to an invalid state to make the first 
    // checking fail intentionally
    unsigned long flush_pg_buf_aligned, flush_pg_buf_offset, last_flush_pg_buf_aligned = INVALID_LPN;
    unsigned long nand_pg_buf_aligned, nand_pg_buf_offset, last_nand_pg_buf_aligned = INVALID_LPN;
    unsigned char flag;

    unsigned long lpa;
    unsigned long src, dest;
    for (lpa = start_lpa; lpa < start_lpa + size; lpa += LOG_DSIZE_GRANULARITY) {
        dest = (unsigned long) data + lpa - start_lpa;
        struct indirection_mte *mte = imt_get(ssd, lpa);
        flag = mte->flag;
        if (mte) {
            // in indirection buffer or flushed before
            if (flag & IN_LOG) {
                // entry is in log
                struct log_entry *loge = (struct log_entry *) cache_mapped(ssd->bd, mte->addr);
                src = (unsigned long) &loge->data_entry.content;
            } else {
                // entry is in nand flash
                flush_pg_buf_aligned = mte->addr & (~PG_MASK);
                flush_pg_buf_offset = mte->addr & PG_MASK;
                if (flush_pg_buf_aligned != last_flush_pg_buf_aligned) {
                    // current page in buffer should be changed
                    internal_transfer_metadata_handle(ssd, mte->addr, 0);
                    backend_rw(ssd->bd, flush_pg_buf_aligned / PG_SIZE, 
                               ssd->flushed_page_buf, 0);
                    last_flush_pg_buf_aligned = flush_pg_buf_aligned;
                }
                src = (unsigned long) ssd->flushed_page_buf + flush_pg_buf_offset;
            }
        } else {
            // in nand flash
            nand_pg_buf_aligned = mte->addr & (~PG_MASK);
            nand_pg_buf_offset = mte->addr & PG_MASK;
            // current page in buffer should be changed
            if (nand_pg_buf_aligned != last_nand_pg_buf_aligned) {
                struct ppa ppa = get_maptbl_ent(ssd, nand_pg_buf_aligned);
                ftl_assert(mapped_ppa(&ppa));
                internal_transfer_metadata_handle(ssd, ppa.realppa, 0);
                backend_rw(ssd->bd, ppa.realppa, ssd->nand_page_buf, 0);
                last_nand_pg_buf_aligned = nand_pg_buf_aligned;
            }
            src = (unsigned long) ssd->nand_page_buf + nand_pg_buf_offset;
        }
        cache_rw(ssd->bd, src, (void *) dest, 0, LOG_DSIZE_GRANULARITY);
    }
}

static void migrate_log(struct ssd *ssd, int num_entries) {
    printk(KERN_NOTICE "Migrate log on %d flushed pages\n", ssd->flushed_arr_size);
    struct nand_cmd ncmd;
    // struct timespec start_time;

    // get migration buffer
    struct log_entry *entries = (struct log_entry *) cache_mapped(ssd->bd, MIGRATION_LOG_BUF_START);
    ftl_assert(entries);
    int entry_idx;
    for (entry_idx = 0; entry_idx < num_entries; entry_idx++) {
        // get next flushed page to operate on
        unsigned long ppa = ssd->flushed_arr[ssd->flushed_arr_head];
        advance_flushed_arr_head(ssd);

        // read the flushed compacted log entries
        internal_transfer_metadata_handle(ssd, ppa, 0);
        backend_rw(ssd->bd, ppa, (void *) entries, 0);
        // mark the ppa as invalid
        struct ppa ppa_struct;
        ppa_struct.realppa = ppa;
        pgidx2ppa(ssd, &ppa_struct);
        mark_page_invalid(ssd, &ppa_struct);

        // last_lpa_aligned is set to an invalid state to make the first 
        // checking fail intentionally
        unsigned long lpa_aligned, lpa_offset, last_lpa_aligned = INVALID_LPN;
        unsigned int i;
        for (i = 0; i < NUM_LOG_PER_PG; i++) {
            lpa_aligned = entries[i].lpa & (~PG_MASK);
            lpa_offset = entries[i].lpa & PG_MASK;
            if (last_lpa_aligned != lpa_aligned) {
                struct ppa ppa;
                // save old page buffer into ssd
                if ((last_lpa_aligned & PG_MASK) == 0) {
                    ppa = get_maptbl_ent(ssd, last_lpa_aligned);
                    if (mapped_ppa(&ppa)) {
                        // update old page information first
                        mark_page_invalid(ssd, &ppa);
                        set_rmap_ent(ssd, INVALID_LPN, &ppa);
                    }
                    // new write
                    ppa = get_new_page(ssd);
                    ssd_advance_write_pointer(ssd);
                    // record new page and mark as valid
                    set_maptbl_ent(ssd, last_lpa_aligned / PG_SIZE, &ppa);
                    set_rmap_ent(ssd, last_lpa_aligned, &ppa);
                    pgidx2ppa(ssd, &ppa);
                    mark_page_valid(ssd, &ppa);

                    internal_transfer_metadata_handle(ssd, ppa.realppa, 1);
                    backend_rw(ssd->bd, ppa.realppa, ssd->migration_dest_buf, 1);
                }
                // bring new page into page buffer
                ppa = get_maptbl_ent(ssd, lpa_aligned);
                if (mapped_ppa(&ppa)) {
                    // exist in nand flash
                    internal_transfer_metadata_handle(ssd, ppa.realppa, 0);
                    backend_rw(ssd->bd, ppa.realppa, ssd->migration_dest_buf, 0);
                } else {
                    // new data, not in nand flash before
                }
                last_lpa_aligned = lpa_aligned;
            }
            // update content
            memcpy(ssd->migration_dest_buf + lpa_offset, 
                entries[i].data_entry.content, 
                LOG_DSIZE_GRANULARITY);
        }
    }
}

static void flush_log(struct ssd *ssd, int num_pages) {
    // TODO: currently without any coalescing
    unsigned avail_num_pages = ssd->log_size / NUM_LOG_PER_PG;
    ftl_assert(num_pages < avail_num_pages);
    printk(KERN_NOTICE "Flush generated %d more NAND pages\n", num_pages);
    unsigned int i,j;
    for (i = 0; i < num_pages; i++) {
        // find a new ppa and fill the log into that space
        struct ppa ppa;
        ppa = get_new_page(ssd);
        ssd_advance_write_pointer(ssd);
        // gather log entries
        for (j = 0; j < NUM_LOG_PER_PG; j++) {
            // fill indirection mapping
            struct log_entry *entry = (struct log_entry *) 
                    cache_mapped(ssd->bd, LOG_START + ssd->log_head * sizeof(struct log_entry));
            ftl_assert(entry);
            // fill in flushed page
            unsigned long in_page_offset = j * sizeof(struct log_entry);
            unsigned long dest = (unsigned long) ssd->flush_wr_page_buf + in_page_offset;
            memcpy((void *) dest, entry, sizeof(struct log_entry));
            // update indirection mapping table and log buffer
            unsigned long mapped_ppa = ppa.realppa * PG_SIZE + in_page_offset;
            imt_insert(ssd, entry->lpa, mapped_ppa);
            imt_remove_flag(ssd, entry->lpa, IN_LOG);
            advance_log_head(ssd);
        }
        // mapping table and remapping table should not be updated
        pgidx2ppa(ssd, &ppa);
        mark_page_valid(ssd, &ppa);
        // but record to the flushed array instead
        ssd->flushed_arr[ssd->flushed_arr_tail] = ppa.realppa;
        advance_flushed_arr_tail(ssd);

        // actual data write
        internal_transfer_metadata_handle(ssd, ppa.realppa, 1);
        backend_rw(ssd->bd, ppa.realppa, ssd->flush_wr_page_buf, 1);
        // migrate upon certain threshold
        if (10 * ssd->flushed_arr_size >= 8 * FLUSHED_ARRAY_ENTRIES) {
            migrate_log(ssd, ssd->flushed_arr_size / 2);
        }
    }
}

static void fill_next_log(struct ssd *ssd, struct log_info *info) {
    unsigned data_offset;
    if (info->type == LOG_DATA) {
        printk(KERN_NOTICE "Fill log DATA Size: %ld\n", info->data_info.data_size);
        ftl_assert(info->data_info.data_size % LOG_DSIZE_GRANULARITY == 0);
        for (data_offset = 0; 
             data_offset < info->data_info.data_size; 
             data_offset += LOG_DSIZE_GRANULARITY) {
            // get current log offset and retrieve corresponding log entry
            unsigned long log_offset = LOG_START + ssd->log_tail * sizeof(struct log_entry);
            struct log_entry *loge = (struct log_entry *) cache_mapped(ssd->bd, log_offset);
            ftl_assert(loge);
            // fill in the log entry with data-specific operations
            loge->lpa = info->actual_lpa;
            memcpy(loge->data_entry.content, info->data_info.data_start + data_offset, LOG_DSIZE_GRANULARITY);
            // update indirection mapping table
            struct indirection_mte *mte = imt_get(ssd, loge->lpa);
            if (mte) {
                // exist, update
                ftl_assert(mte->lpa == loge->lpa);
                mte->addr = (unsigned long) loge;
                if (mte->flag & IN_LOG) {
                    // exist in log
                } else {
                    // exist in flushed pages
                    mte->flag |= IN_LOG;
                }
            } else {
                // does not exist, append
                imt_insert(ssd, loge->lpa, log_offset);
                imt_set_flag(ssd, loge->lpa, IN_LOG);
                ssd->indirection_mt_size++;
            }
            advance_log_tail(ssd);
            // printf("Capacity: %f%%\n", (double) ssd->log_size * 100 / LOG_ENTRIES);
            // sleepns(1000000);
            // flush upon log fill to certain capacity
            if (10 * ssd->log_size >= 5 * LOG_ENTRIES) {
                flush_log(ssd, ssd->log_size / NUM_LOG_PER_PG / 2);
            }
        }
    } else if (info->type == LOG_METADATA) {
        unsigned long offset = LOG_START + ssd->log_tail * sizeof(struct log_entry);
        struct log_entry *e = (struct log_entry *) cache_mapped(ssd->bd, offset);
        e->lpa = info->actual_lpa;
        // TODO: complete this
    } else {
        ftl_assert(false);
    }
}

static uint64_t ssd_read(struct ssd *ssd, event *ctl)
{
    NvmeRwCmd* req = (NvmeRwCmd*)(&(ctl->cmd));
    struct ssdparams *spp = &ssd->sp;
    //@16 kb page or change pgsz to 4kb
    uint64_t lba = req->slba;
    int nlb = req->nlb;
    struct ppa ppa;
    // I think the nvme granularity here is 4K? need to make sure it is not 512B (sector size)
    uint64_t start_lpn = lba;
    uint64_t end_lpn = lba + nlb;
    // uint64_t end_lpn = lba + nlb;
    uint64_t lpn;
    uint64_t sublat, maxlat = 0;
    // printf("read request: lpn[%lu,%lu)\n", start_lpn, end_lpn);
    if (end_lpn >= spp->tt_pgs) {
        // ftl_err("start_lpn=%"PRIu64",tt_pgs=%d\n", start_lpn, ssd->sp.tt_pgs);

    }

    /* normal IO read path */
    for (lpn = start_lpn; lpn < end_lpn; lpn++)   {
        ppa = get_maptbl_ent(ssd, lpn);
        if (!mapped_ppa(&ppa) || !valid_ppa(ssd, &ppa)) {
            //printf("%s,lpn(%" PRId64 ") not mapped to valid ppa\n", ssd->ssdname, lpn);
            //printf("Invalid ppa,ch:%d,lun:%d,blk:%d,pl:%d,pg:%d,sec:%d\n",
            //ppa.g.ch, ppa.g.lun, ppa.g.blk, ppa.g.pl, ppa.g.pg, ppa.g.sec);
            continue;
        }
        // printf("(ReadReq: lpn=%lu, ppa=%lu) ", lpn, ppa.realppa);

        struct nand_cmd srd;
        srd.type = USER_IO;
        srd.cmd = NAND_READ;
        srd.stime = ctl->s_time;
        sublat = ssd_advance_status(ssd, &ppa, &srd);
        // max latency is the time all pages finishes reading (reading is not posted)
        // printf("read request: sublat=%lu\n", sublat);
        maxlat = (sublat > maxlat) ? sublat : maxlat;
        //actually read page into addr @TODO check it this is safe
        int result = backend_rw(ssd->bd, ppa.realppa, (void *) req->prp1, 0);
    }
    // printf("\n");

    return maxlat;
}



static uint64_t ssd_write(struct ssd *ssd, event *ctl)
{
    NvmeRwCmd* req = (NvmeRwCmd*)(&(ctl->cmd));
    uint64_t lba = req->slba;
    struct ssdparams *spp = &ssd->sp;
    int nlb = req->nlb;
    uint64_t start_lpn = lba;
    uint64_t end_lpn = lba + nlb;
    struct ppa ppa;
    uint64_t lpn;
    uint64_t curlat = 0, maxlat = 0;
    int r;
    // printf("write request: lpn[%lu,%lu)\n", start_lpn, end_lpn);
    if (end_lpn >= spp->tt_pgs) {
        // ftl_err("start_lpn=%"PRIu64",tt_pgs=%d\n", start_lpn, ssd->sp.tt_pgs);
    }

    // while (should_gc_high(ssd)) {
    //     /* perform GC here until !should_gc(ssd) */
    //     r = do_gc(ssd, true);
    //     if (r == -1)
    //         break;
    // }

    for (lpn = start_lpn; lpn < end_lpn; lpn++) {
        ppa = get_maptbl_ent(ssd, lpn);
        if (mapped_ppa(&ppa)) {
            /* update old page information first */
            mark_page_invalid(ssd, &ppa);
            set_rmap_ent(ssd, INVALID_LPN, &ppa);
        }

        /* new write */
        ppa = get_new_page(ssd);

        // printf("write request: lpn=%lu, ppa=%lu", lpn, ppa.realppa);
        /* update maptbl */
        set_maptbl_ent(ssd, lpn, &ppa);
        /* update rmap */
        set_rmap_ent(ssd, lpn, &ppa);

        // don't need to mark valid here?
        mark_page_valid(ssd, &ppa);

        /* need to advance the write pointer here */
        ssd_advance_write_pointer(ssd);

        struct nand_cmd swr;
        swr.type = USER_IO;
        swr.cmd = NAND_WRITE;
        swr.stime = ctl->s_time;
        /* get latency statistics */
        curlat = ssd_advance_status(ssd, &ppa, &swr);
        maxlat = (curlat > maxlat) ? curlat : maxlat;
        //actually write page into addr @TODO check it this is safe
        int result = backend_rw(ssd->bd, ppa.realppa, (void *) req->prp1, 1);
    }
    // printf("\n");

    return maxlat;
}

struct ftl_thread_info{
    int num_poller;
    struct Ring **to_ftl;
    struct Ring **to_poller;
};

int ftl_thread(void* arg)
{
    struct ftl_thread_info *info = (struct ftl_thread_info*)arg;
    struct ssd *ssd = &gdev;
    event *ctl_event = NULL;
    NvmeRwCmd *req = NULL;
    uint64_t lat = 0;
    int rc;
    int i;

    // while (!*(ssd->dataplane_started_ptr)) {
    //     usleep(100000);
    // }

    //init time here
    /* measure monotonic time for nanosecond precision */
	// clock_gettime(CLOCK_MONOTONIC, &start);
    start = get_time_ns();

    ssd->to_ftl = info->to_ftl;
    ssd->to_poller = info->to_poller;

    //main loop
    while (1) {
        // keep the multiple poller here for furture expansion of multiple queue
        // printf("ftl thread: waiting for event from poller");
        for (i = 1; i <= info->num_poller; i++) {
            if (!ssd->to_ftl[i] || !ring_is_empty(ssd->to_ftl[i]))
                continue;

            ctl_event = (event *)ring_get(ssd->to_ftl[i]);
            if (!ctl_event) {
                ftl_err("FTL to_ftl dequeue failed %ld\n", ctl_event->identification);
            };



            req = (NvmeRwCmd *)(&(ctl_event->cmd));
            // clock_gettime(CLOCK_MONOTONIC, &cur);
            cur = get_time_ns();

            ctl_event->s_time = cur;

            switch (req->opcode) {
            case NVME_CMD_WRITE:
                lat = ssd_write(ssd, ctl_event);
                break;
            case NVME_CMD_READ:
                lat = ssd_read(ssd, ctl_event);
                break;

                // add future api here
            // case NVME_CMD_DSM:
            //     lat = 0;
            //     break;

            default:
                ftl_err("FTL received unkown request type, ERROR\n");
            }



            ctl_event->reqlat = lat;
            ctl_event->expire_time = ctl_event->s_time + lat;

            rc = ring_put(ssd->to_poller[i], (void *)&ctl_event);

            if (rc == -1) {
                ftl_err("FTL to_poller enqueue failed, %ld\n", ctl_event->identification);
            }

            // /* clean if needed (in the background) */
            // if (should_gc(ssd)) {
            //     do_gc(ssd, false);
            // }
        }
    }

    return NULL;
}

int request_poller_thread(void* arg) {
    struct ftl_thread_info *info = (struct ftl_thread_info*)arg;
    struct ssd *ssd = &gdev;
    event* evt = kzalloc(sizeof(struct cntrl_event),GFP_KERNEL);
    int rc;
    int i;
    while (1) {
        for (i = 1; i <= info->num_poller; i++) {
            if (!ssd->to_poller[i] || !ring_is_empty(ssd->to_poller[i]))
                continue;

            evt = (event*)ring_get(ssd->to_poller[i]);
            if (!evt) {
                printk(KERN_ERR "FTL to_poller dequeue failed %ld\n", evt->identification);
            }
            ftl_assert(evt);
            evt->completed = 1;
        }

    }   
    return 0;
}

void ssd_init(void)
{
    struct ssd *ssd = &gdev;
    struct ssdparams *spp = &ssd->sp;

    ftl_assert(ssd);

    ssd_init_params(spp);

    /* initialize ssd internal layout architecture */
    ssd->ch = kzalloc(sizeof(struct ssd_channel) * spp->nchs,GFP_KERNEL);
    int i;
    for (i = 0; i < spp->nchs; i++) {
        ssd_init_ch(&ssd->ch[i], spp);
    }

    /* initialize maptbl */
    ssd_init_maptbl(ssd);

    /* initialize rmap */
    ssd_init_rmap(ssd);

    // /* initialize all the lines */
    // ssd_init_lines(ssd);

    /* initialize write pointer, this is how we allocate new pages for writes */
    ssd_init_write_pointer(ssd);

    //init all queues
    ssd_init_queues(ssd);

    //init backend
    int ret = init_dram_backend(&(ssd->bd), TOTAL_SIZE, 0);

    // initialize indirection mapping table
    ssd->indirection_mt = cache_mapped(ssd->bd, INDIRECTION_MT_START);
    ssd->log_head = 0;
    ssd->log_tail = 0;
    ssd->log_size = 0;
    ssd->flushed_arr = cache_mapped(ssd->bd, FLUSHED_ARRAY_START);
    ssd->flushed_arr_head = 0;
    ssd->flushed_arr_tail = 0;
    ssd->flushed_arr_size = 0;

    // initialize intermediate buffers
    ssd->flush_wr_page_buf = cache_mapped(ssd->bd, PG_WR_FLUSH_BUF_START);
    ssd->flushed_page_buf = cache_mapped(ssd->bd, PG_RD_FLUSH_BUF_START);
    ssd->nand_page_buf = cache_mapped(ssd->bd, PG_RD_NAND_BUF_START);
    ssd->migration_dest_buf = cache_mapped(ssd->bd, MIGRATION_DEST_BUF_START);
    ssd->migration_log_buf = cache_mapped(ssd->bd, MIGRATION_LOG_BUF_START);
    



    // init new kernel thread for ftl emulation
    ssd->thread_id = kzalloc(sizeof(struct task_struct ),GFP_KERNEL);
    struct ftl_thread_info *arg = kzalloc(sizeof(struct ftl_thread_info),GFP_KERNEL);
    arg->num_poller = 1;
    arg->to_ftl = ssd->to_ftl;
    arg->to_poller = ssd->to_poller;

    // ftl_thread(arg);
    ssd->thread_id = kthread_create(ftl_thread, arg, "ftl_thread");
    // ret = pthread_create(ssd->thread_id, NULL, ftl_thread, arg);
   


    // qemu_thread_create(&ssd->ftl_thread, "FEMU-FTL-Thread", ftl_thread, n,
    //                    QEMU_THREAD_JOINABLE);
    if (ssd->thread_id == 0)
        ftl_err("Failed to create FTL thread\n");
    else{
        printk(KERN_NOTICE "FTL thread created successfully\n");
        wake_up_process(ssd->thread_id);
    }

    ssd->polling_thread_id = kzalloc(sizeof(struct task_struct),GFP_KERNEL);
    ssd->polling_thread_id = kthread_create(request_poller_thread, arg, "ftl_poller_thread");
    if (ssd->polling_thread_id == 0)
        ftl_err("Failed to create request poller thread\n");
    else{
        printk(KERN_NOTICE "Request poller thread created successfully\n");
        wake_up_process(ssd->polling_thread_id);
    }
    // ret = pthread_create(ssd->polling_thread_id, NULL, request_poller_thread, arg);
    
    // sleep(20);
}


/** user block interface to init an nvme cmd to the ssd device */
int nvme_issue(int is_write, uint64_t lba, uint64_t len, void *buf){

    // uint64_t abs_wait_barrier, actural_barrier;

    event* e = kzalloc(sizeof(struct cntrl_event),GFP_KERNEL);
    if (is_write) {
        bytefs_init_nvme(&(e->cmd),NVME_CMD_WRITE,lba,len,buf);
    } else {
        bytefs_init_nvme(&(e->cmd),NVME_CMD_READ,lba,len,buf);
    }
    
    e->s_time = 0;
    e->expire_time = 0;
    e->reqlat = 0;
    e->completed =0;
    e->e_time = 0;
    // e->identification = rand();
    struct ssd *ssd = &gdev;
    int ret = ring_put(ssd->to_ftl[1], (void *)&e);
    if (!ret) {
        printk(KERN_ERR "ring buffer full, wait for space\n");
        // while (!ring_put(ssd->to_ftl[1], (void *)&e)) {}
    }
    while (!e->completed) {}
    sleepns(e->expire_time - 65000 - e->s_time);

    // uint2time(e->expire_time - 65000 - e->s_time, &abs_wait_barrier);
    // printf("wait for barrier %llu\n",e->expire_time);
    // clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &abs_wait_barrier, NULL);

    // clock_gettime(CLOCK_MONOTONIC, &actural_barrier);

    uint64_t actrual_t = get_time_ns();

    // uint64_t actrual_t = time2uint(&actural_barrier);
    printk(KERN_NOTICE "FIN: <%ld> @ %ld(%ld) | %ld(%ld) => %ld\n", e->identification, 
            e->expire_time, actrual_t, 
            e->expire_time - e->s_time, actrual_t - e->s_time,
            actrual_t - e->expire_time);
    kfree(e);
    return 0;
}

int byte_issue(int is_write, uint64_t lpa, uint64_t size, void *buf) {
    struct ssd *ssd = &gdev;
    struct log_info info;
    if (is_write) {
        info.actual_lpa = lpa;
        info.type = LOG_DATA;
        info.data_info.data_start = buf;
        info.data_info.data_size = size;
        fill_next_log(ssd, &info);
    } else {
        read_data(ssd, lpa, size, buf);
    }
    return 0;
}

// // test only
// int main() {
//     printf("Total DRAM Cache Buffer Size: %ldB %fMB\n", 
//             DRAM_CACHE_END, (double) DRAM_CACHE_END / 1024 / 1024);
//     printf("LOG ENTRY SIZE: %ld, LOG TABLE SIZE %d\n", sizeof(struct log_entry), LOG_SIZE);

//     ssd_init();
//     // char add[PG_SIZE * 200] = "444";
//     // char add2[PG_SIZE * 200] = "";
    
    
//     // struct ssd *ssd = &gdev;
//     // struct ppa ppa = get_new_page(ssd);
//     // mark_page_valid(ssd, &ppa);
//     // mark_page_invalid(ssd, &ppa);
    
//     // exit(0);
    
//     unsigned long workset_size = PG_SIZE * 2000;
//     char *garbage = (char *) kzalloc(workset_size);
//     for (int i = 0; i < PG_SIZE; i++) {
//         memset(garbage, i % 256, workset_size / PG_SIZE);
//     }

//     byte_issue(1, 0, workset_size, garbage);
//     for (int i = 0; i < 1000000; i++) {
//         unsigned long byte_size = (rand() * LOG_DSIZE_GRANULARITY) % PG_SIZE;
//         unsigned long lpa = rand() % (workset_size - byte_size);
//         // byte_issue(1, )
//         // nvme_issue(1, rand(), rand() % 10, garbage[rand()]);
//     }

//     return 0;
// }
