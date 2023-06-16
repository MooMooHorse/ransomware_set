#include "diag_syscall.h"

#define SEC_TO_BYTES(sec)  ((sec) << 9)
#define SEC_SIZE           512

#define NUM2CHAR(num)      ((char)(num&0xff))

#define ENCRYPT(byte)      (~(byte))

diag_ctrl_t diag_ctrl = {0, {NULL}, 0 , NULL, 0};
bio_cache_t bio_cache = {0, 0, 0, 0, RB_ROOT, UTF8, 0, 0, __SPIN_LOCK_UNLOCKED(bio_cache.lock)};

static inline cache_entry_t* get_ceh(struct rb_node* node) {
    return container_of(node, cache_entry_t, node);
}

static inline cache_entry_t* buf_alloc(uint64_t lpa, int is_encrypted) {
    cache_entry_t* ret;
    ret = kmalloc(sizeof(cache_entry_t), GFP_KERNEL);
    if(NULL == ret) {
        printk(KERN_ERR "kmalloc failed at %s:%d\n", __FILE__, __LINE__);
        return NULL;
    }
    ret->lpa = lpa;
    ret->is_encrypted = is_encrypted;
    return ret;
}


int64_t rb_update(struct rb_root* root, uint64_t lpa, struct rb_node** node, 
uint64_t is_encrypted) {
    struct rb_node** _new;
    struct rb_node* parent = NULL;
    int64_t ret = -1;

    if(NULL == root) return -1;

    _new = &(root->rb_node);

    while(*_new) {
        cache_entry_t* ceh = get_ceh(*_new);
        parent = *_new;
        if(lpa < ceh->lpa) {
            _new = &((*_new)->rb_left);
        } else if(lpa > ceh->lpa) {
            _new = &((*_new)->rb_right);
        } else {
            bio_cache.unencrypted_len -= ceh->is_encrypted ? 0 : 1;
            bio_cache.encrypted_len -= ceh->is_encrypted ? 1 : 0;
            ceh->is_encrypted = is_encrypted;
            bio_cache.unencrypted_len += ceh->is_encrypted ? 0 : 1;
            bio_cache.encrypted_len += ceh->is_encrypted ? 1 : 0;
            return 0;
        }
    }

    if(NULL == *_new) {
        cache_entry_t* ceh = buf_alloc(lpa, is_encrypted);
        diag_ctrl.blk2file_size++;
        if(NULL == ceh) {
            printk(KERN_ERR "buf_alloc failed at %s:%d\n", __FILE__, __LINE__);
            return -1;
        }
        bio_cache.unencrypted_len += ceh->is_encrypted ? 0 : 1;
        bio_cache.encrypted_len += ceh->is_encrypted ? 1 : 0;
        // add it in the rb tree
        rb_link_node(&(ceh->node), parent, _new);
        rb_insert_color(&(ceh->node), root);
        *node = &(ceh->node);
        ret = 0;
    } else {
        printk(KERN_ERR "rb_tree_alloc_and_insert failed at %s:%d\n", __FILE__, __LINE__);
        ret = -1;
    }
    return ret;
}


// delete whole rb tree, time complexity O(n)
// @note : caller needs to use spin_lock to protect the rb tree
static void _delete_rb_tree(struct rb_node* node) {
    if(NULL == node) return;
    delete_rb_tree(node->rb_left);
    delete_rb_tree(node->rb_right);
    kfree(get_ceh(node));
}

void delete_rb_tree(struct rb_root* root) {
    if(NULL == root) return;
    _delete_rb_tree(root->rb_node);
    root->rb_node = NULL;
}



static void clear_cache() {
    bio_cache.unencrypted_len = 0;
    bio_cache.encrypted_len = 0;
    delete_rb_tree(&bio_cache.cache_root);
    bio_cache.cache_root = RB_ROOT;
    diag_ctrl.blk2file_size = 0;
}


/**
 * set disks to monitor.
 * @return 0 if success, -1 if fail
*/
int set_disks(uint64_t num_disks, char** disks) {
    if(num_disks > MAX_DISKS) {
        printk(KERN_ERR "too many disks\n");
        return -1;
    }
    diag_ctrl.num_disk = num_disks;
    for (int i = 0; i < num_disks; i++) {
        diag_ctrl.monitored_disk[i] = kmalloc(sizeof(char) * 256, GFP_KERNEL);
        if (diag_ctrl.monitored_disk[i] == NULL) {
            printk(KERN_ERR "kmalloc failed\n");
            return -1;
        }
        strcpy(diag_ctrl.monitored_disk[i], disks[i]);
    }
    return 0;
}



// return 1 if encrypted, 0 if unencrypted
int get_encrpted(const char* buf, uint64_t sec_off) {
    int ret = 0;
    uint32_t i;
    uint64_t big_magic, target_magic;
    uint64_t _magic1, _magic2, _magic3, _magic4;
    target_magic = (ENCRYPT(bio_cache.magic) << 24) | (ENCRYPT(bio_cache.magic) << 16) | 
    (ENCRYPT(bio_cache.magic) << 8) | ENCRYPT(bio_cache.magic);

    for(i = SEC_TO_BYTES(sec_off); i < SEC_TO_BYTES(sec_off) + SEC_SIZE; i += this->min_len) {
        _magic1 = buf[i]; _magic2 = buf[i + 1]; _magic3 = buf[i + 2]; _magic4 = buf[i + 3];
        big_magic = (_magic1 << 24) | (_magic2 << 16) | (_magic3 << 8) | _magic4;
        ret += (big_magic == target_magic) * this->min_len;
        if(ret >= DYE_THRESHOLD) {
            return 1;
        }
    }
    return 0;
}


// return 1 if unencrypted, 0 if encrypted
int get_unencrpted(const char* buf, uint64_t sec_off) {
    int ret = 0;
    uint32_t i;
    uint64_t big_magic, target_magic;
    uint64_t _magic1, _magic2, _magic3, _magic4;
    target_magic = ((bio_cache.magic) << 24) | ((bio_cache.magic) << 16) | 
    ((bio_cache.magic) << 8) | (bio_cache.magic);

    for(i = SEC_TO_BYTES(sec_off); i < SEC_TO_BYTES(sec_off) + SEC_SIZE; i += this->min_len) {
        _magic1 = buf[i]; _magic2 = buf[i + 1]; _magic3 = buf[i + 2]; _magic4 = buf[i + 3];
        big_magic = (_magic1 << 24) | (_magic2 << 16) | (_magic3 << 8) | _magic4;
        ret += (big_magic == target_magic) * this->min_len;
        if(ret >= DYE_THRESHOLD) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief :
 *  process bio
 * RETURN:
 * number of 4KB pages issued through one call of byte_fs_bio_issue()
 * 
 * SIDE-EFFECT: 
 * spinlock might cause deadlock (not fully tested)
 * 
 * NOTE : 
 * The nvme issue is protected (in current version) by spinlock with irq save since the context is not clear.
 * The necessity of spinlock is unsure, and open to further testing.
 * 
*/
int diag_proc_bio(struct bio* bio){
    
    if(diag_ctrl.trace_status == 0) return 0;

	int v_idx;
	char* virt_pg_addr;
	uint64_t lba=bio->bi_iter.bi_sector/8;
	uint64_t flags;
	unsigned long* if_end_io;
	unsigned short tot_vcnt = bio->bi_vcnt; // used to eliminate danger after bi_end_io
	int ret=0,fret;

    int encrypted_len, unencrypted_len;

    int cnt;

    // uint64_t time_us;

    // time_us = ktime_to_us(ktime_get());
	
    uint64_t lsa = bio->bi_iter.bi_sector;
    ret = 0;
    total_issued_bytes = 0;
    for(v_idx = 0; v_idx < bio->bi_vcnt; v_idx++) {
        virt_pg_addr = ((char*)page_address(bio->bi_io_vec[v_idx].bv_page) + bio->bi_io_vec[v_idx].bv_offset);

        for(cnt = 0; cnt < bio->bi_io_vec[v_idx].bv_len / SEC_SIZE; cnt++) {
            encrypted_len = get_encrpted(virt_pg_addr, cnt * SEC_SIZE);
            unencrypted_len = get_unencrpted(virt_pg_addr, cnt * SEC_SIZE);
            
            /* sanity check 
                * assumption here is before ransomware is turned on, no encrypted content should be detected
                * and when ransomware is turned on, no unencrypted content should be detected 
                * (because in user space, we deliberately do a sync before turning on ransomware)
                * note that those are only true for testing files and this function should be used in ransomare testing framework only
            */
            if(bio_cache.rans_enabled && unencrypted_len) {
                printk(KERN_INFO "ransomware detected but not encrypted? at %s:%d\n", __FILE__, __LINE__);
            }
            if(bio_cache.rans_enabled && encrypted_len) {
                printk(KERN_INFO "ransomware not detected but encrypted? at %s:%d\n", __FILE__, __LINE__);
            }

            if(!encrypted_len && !unencrypted_len) {
                continue;
            }
            struct rb_node *node;
            spin_lock_irqsave(&bio_cache.lock, flags);
            if(-1 == rb_update(&bio_cache.cache_root, lsa++, &node, encrypted_len)) {
                spin_unlock_irqrestore(&bio_cache.lock, flags);
                break;
            }
            spin_unlock_irqrestore(&bio_cache.lock, flags);
        }
        // if(0 > (fret = add_trace(time_us, op_is_write(bio_op(bio)), lsa, bio->bi_io_vec[v_idx].bv_len / 512,
        // encrypted_len, unencrypted_len)
        // )) {
        //     spin_unlock_irqrestore(&bio_cache.lock,flags);
        //     printk(KERN_ERR "add trace failed at %s:%d\n", __FILE__, __LINE__);
        //     return fret;
        // }
        ret += fret;
    }
    bio_endio(bio);
    return ret;

}	

uint64_t get_blk2file_size() {
    return diag_ctrl.blk2file_size;
}

void get_blk2file(uint64_t* __user buf) {
    struct rb_node *node;
    if(diag_ctrl.blk2file_size < (1<<12)) {
        diag_ctrl.blk2file = vmalloc(sizeof(uint64_t) * (1<<12));
    } else {
        diag_ctrl.blk2file = vmalloc(((sizeof(uint64_t) * diag_ctrl.blk2file_size) >> 12) << 12);
    }
    for(node = rb_first(&bio_cache.cache_root); node; node = rb_next(node)) {
        cache_entry_t* ceh = get_ceh(node);
        diag_ctrl.blk2file[diag_ctrl.blk2file_size++] = ceh->lsa;
    }
    vfree(diag_ctrl.blk2file);
}

void clear_all() {
    // clear_trace();
    clear_cache();
}

void turn_on_rans() {
    bio_cache.rans_enabled = 1;
}

void turn_off_rans() {
    bio_cache.rans_enabled = 0;
}

void turn_on_trace() {
    diag_ctrl.trace_status = 1;
}

void turn_off_trace() {
    diag_ctrl.trace_status = 0;
}

int is_trace_on() {
    return diag_ctrl.trace_status;
}


/**
 * @brief add a trace to trace buffer
 * @side-effect: 
 *  If trace buffer is unallocated, alloc a new one.
 *  If dump_real_trace is not enabled, do nothing.
 * @return number of traces added
*/
// int add_trace(uint64_t time, io_type_t IO_type, uint64_t lba, uint64_t len, uint64_t unencrypted_len, uint64_t encrypted_len) {
//     if(!diag_ctrl.dump_real_trace) return 0;
//     diag_ctrl_entry_t* trace = get_trace();
//     if (trace == NULL) {
//         if(NULL == alloc_trace()){
//             printk(KERN_ERR "alloc_trace failed at %s:%d\n", __FILE__, __LINE__);
//             return -1;
//         }
//     }
//     trace[diag_ctrl.num_trace].time = time;
//     trace[diag_ctrl.num_trace].IO_type = IO_type;
//     trace[diag_ctrl.num_trace].lba = lba;
//     trace[diag_ctrl.num_trace].len = len;
//     trace[diag_ctrl.num_trace].unencrypted_len = unencrypted_len;
//     trace[diag_ctrl.num_trace++].encrypted_len = encrypted_len;
//     return 1;
// }

// uint64_t get_num_trace() {
//     return diag_ctrl.num_trace;
// }


// diag_ctrl_entry_t* alloc_trace() {
//     diag_ctrl.trace = vmalloc(sizeof(diag_ctrl_entry_t) * MAX_NUM_TRACE);
//     diag_ctrl.num_trace = 0;
//     return diag_ctrl.trace;
// }

// diag_ctrl_entry_t* get_trace() {
//     return diag_ctrl.trace;
// }

// static void clear_trace() {
//     diag_ctrl.num_trace = 0;
// }
