#include "diag_syscall.h"

#define SEC_TO_BYTES(sec)  ((sec) << 9)
#define SEC_SIZE           512

#define NUM2CHAR(num)      ((char)(num&0xff))

#define ENCRYPT(byte)      (~(byte))

diag_ctrl_t diag_ctrl = {NULL, 0, 0, 0, {NULL}};
bio_cache_t bio_cache = {0, 0, 0, 0, RB_ROOT, UTF8, __SPIN_LOCK_UNLOCKED(bio_cache.lock)};

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

void delete_rb_tree(struct rb_root* root) {
    struct rb_node* node = root->rb_node;
    while(node) {
        struct rb_node* tmp = node;
        node = node->rb_left;
        kfree(get_ceh(tmp));
        rb_erase(tmp, root);
    }
}

diag_ctrl_entry_t* alloc_trace() {
    diag_ctrl.trace = kmalloc(sizeof(diag_ctrl_entry_t) * MAX_NUM_TRACE, GFP_KERNEL);
    return diag_ctrl.trace;
}

diag_ctrl_entry_t* get_trace() {
    return diag_ctrl.trace;
}

static void clear_trace() {
    diag_ctrl.num_trace = 0;
}

static void clear_cache() {
    bio_cache.unencrypted_len = 0;
    bio_cache.encrypted_len = 0;
    delete_rb_tree(&bio_cache.cache_root);
    bio_cache.cache_root = RB_ROOT;
}

void clear_all() {
    clear_trace();
    clear_cache();
}

uint64_t get_num_trace() {
    return diag_ctrl.num_trace;
}

void turn_on_trace() {
    diag_ctrl.trace_status = 1;
}

void turn_off_trace() {
    diag_ctrl.trace_status = 0;
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

int is_trace_on() {
    return diag_ctrl.trace_status;
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
