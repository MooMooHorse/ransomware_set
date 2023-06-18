#include "diag_syscall.h"

#define SEC_TO_BYTES(sec)  ((sec) << 9)
#define SEC_SIZE           512

#define NUM2CHAR(num)      ((char)(num&0xff))

#define ENCRYPT(byte)      (~(byte))

diag_ctrl_t diag_ctrl = {0 , 0, {NULL}, 0, NULL, 0};
bio_cache_t bio_cache = {0, 0, 4, RB_ROOT, 0, 0, __SPIN_LOCK_UNLOCKED(bio_cache.lock)};

static inline cache_entry_t* get_ceh(struct rb_node* node) {
    return container_of(node, cache_entry_t, node);
}

static inline cache_entry_t* buf_alloc(uint64_t lsa, int is_encrypted) {
    cache_entry_t* ret;
    ret = kmalloc(sizeof(cache_entry_t), GFP_KERNEL);
    if(NULL == ret) {
        printk(KERN_ERR "kmalloc failed at %s:%d\n", __FILE__, __LINE__);
        return NULL;
    }
    ret->lsa = lsa;
    ret->is_encrypted = is_encrypted;
    return ret;
}


int64_t rb_update(struct rb_root* root, uint64_t lpa, struct rb_node** node, uint64_t is_encrypted) {
    struct rb_node** _new;
    struct rb_node* parent = NULL;
    int64_t ret = -1;

    if(NULL == root) return -1;

    _new = &(root->rb_node);

    while(*_new) {
        cache_entry_t* ceh = get_ceh(*_new);
        parent = *_new;
        if(lpa < ceh->lsa) {
            _new = &((*_new)->rb_left);
        } else if(lpa > ceh->lsa) {
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
        if(NULL == ceh) {
            printk(KERN_ERR "buf_alloc failed at %s:%d\n", __FILE__, __LINE__);
            return -1;
        }
        diag_ctrl.blk2file_size++;
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
    _delete_rb_tree(node->rb_left);
    _delete_rb_tree(node->rb_right);
    kfree(get_ceh(node));
}

void delete_rb_tree(struct rb_root* root) {
    if(NULL == root) return;
    _delete_rb_tree(root->rb_node);
    root->rb_node = NULL;
}



static void clear_cache(void) {
    uint64_t flags;
    bio_cache.unencrypted_len = 0;
    bio_cache.encrypted_len = 0;
    spin_lock_irqsave(&bio_cache.lock, flags);
    delete_rb_tree(&bio_cache.cache_root);
    bio_cache.cache_root = RB_ROOT;
    spin_unlock_irqrestore(&bio_cache.lock, flags);
    diag_ctrl.blk2file_size = 0;
}


/**
 * set disks to monitor.
 * @return 0 if success, -1 if fail
*/
int set_disks(uint64_t num_disks, char** disks) {
    int i;
    if(num_disks > MAX_DISKS) {
        printk(KERN_ERR "too many disks\n");
        return -1;
    }
    diag_ctrl.num_disk = num_disks;
    for (i = 0; i < num_disks; i++) {
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
    target_magic = (ENCRYPT(diag_ctrl.magic) << 24) | (ENCRYPT(diag_ctrl.magic) << 16) | 
    (ENCRYPT(diag_ctrl.magic) << 8) | ENCRYPT(diag_ctrl.magic);

    for(i = SEC_TO_BYTES(sec_off); i < SEC_TO_BYTES(sec_off) + SEC_SIZE; i += bio_cache.min_len) {
        _magic1 = buf[i]; _magic2 = buf[i + 1]; _magic3 = buf[i + 2]; _magic4 = buf[i + 3];
        big_magic = (_magic1 << 24) | (_magic2 << 16) | (_magic3 << 8) | _magic4;
        ret += (big_magic == target_magic) * bio_cache.min_len;
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
    target_magic = ((diag_ctrl.magic) << 24) | ((diag_ctrl.magic) << 16) | 
    ((diag_ctrl.magic) << 8) | (diag_ctrl.magic);

    for(i = SEC_TO_BYTES(sec_off); i < SEC_TO_BYTES(sec_off) + SEC_SIZE; i += bio_cache.min_len) {
        _magic1 = buf[i]; _magic2 = buf[i + 1]; _magic3 = buf[i + 2]; _magic4 = buf[i + 3];
        big_magic = (_magic1 << 24) | (_magic2 << 16) | (_magic3 << 8) | _magic4;
        ret += (big_magic == target_magic) * bio_cache.min_len;
        if(ret >= DYE_THRESHOLD) {
            return 1;
        }
    }
    return 0;
}

void set_magic(uint64_t magic) {
    diag_ctrl.magic = magic;
}


int diag_proc_bio(struct bio* bio){
	char* virt_pg_addr;
	uint64_t flags;
	int ret;
	int v_idx;
    int encrypted_len, unencrypted_len;
    int cnt;
    uint64_t lsa;

    if(diag_ctrl.trace_status == 0) return 0;

    lsa = bio->bi_iter.bi_sector;
    ret = 0;

    for(v_idx = 0; v_idx < bio->bi_vcnt; v_idx++) {
        virt_pg_addr = ((char*)kmap(bio->bi_io_vec[v_idx].bv_page) + bio->bi_io_vec[v_idx].bv_offset);
        if(virt_pg_addr == NULL) {
            printk(KERN_ERR "kmap failed at %s:%d\n", __FILE__, __LINE__);
            return -1;
        }
        for(cnt = 0; cnt < bio->bi_io_vec[v_idx].bv_len / SEC_SIZE; cnt++) {
            encrypted_len = get_encrpted(virt_pg_addr, cnt);
            unencrypted_len = get_unencrpted(virt_pg_addr, cnt);
            
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
            spin_lock_irqsave(&bio_cache.lock, flags);
            struct rb_node *node;
            if(-1 == rb_update(&bio_cache.cache_root, lsa++, &node, encrypted_len)) {
                printk(KERN_ERR "rb_update failed at %s:%d\n", __FILE__, __LINE__);
                spin_unlock_irqrestore(&bio_cache.lock, flags);
                break;
            }
            spin_unlock_irqrestore(&bio_cache.lock, flags);
            ret ++;
        }
    }
    return ret;

}	

uint64_t get_blk2file_size(void) {
    return diag_ctrl.blk2file_size;
}

void get_blk2file(uint64_t* __user buf) {
    struct rb_node *node;
    int cnt;
    cnt = 0;
    if(diag_ctrl.blk2file_size < (1<<12)) {
        diag_ctrl.blk2file = vmalloc(sizeof(uint64_t) * (1<<12));
    } else {
        diag_ctrl.blk2file = vmalloc(( ((sizeof(uint64_t) * diag_ctrl.blk2file_size) >> 12) + 1 ) << 12);
    }
    for(node = rb_first(&bio_cache.cache_root); node; node = rb_next(node)) {
        cache_entry_t* ceh = get_ceh(node);
        diag_ctrl.blk2file[cnt++] = ceh->lsa;
    }
    copy_to_user(buf, diag_ctrl.blk2file, sizeof(uint64_t) * diag_ctrl.blk2file_size);
    vfree(diag_ctrl.blk2file);
}

void clear_all(void) {
    // clear_trace();
    clear_cache();
}

void turn_on_rans(void) {
    bio_cache.rans_enabled = 1;
}

void turn_off_rans(void) {
    bio_cache.rans_enabled = 0;
}

void turn_on_trace(void) {
    diag_ctrl.trace_status = 1;
}

void turn_off_trace(void) {
    diag_ctrl.trace_status = 0;
}

int is_trace_on(void) {
    return diag_ctrl.trace_status;
}
