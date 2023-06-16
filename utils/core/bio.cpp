/**
 * @file bio.cpp
 * @author Hao Ren (rhao8608@gmail.com)
 * @brief BIO cache is used to help replay BIO operations. It is meant to count the unencrypted and encrypted bytes 
 * left in disk by ransomware attack. 
 * BIO cache utilizes red-black tree to store the unencrypted and encrypted bytes. The cache is lazy updated,
 * and are stored in a priority queue. The priority queue is used to help flush the cache.
 * Encrypted and unencrypted bytes in standardized ransomware is characterized by a magic number, which is
 * magic3 before encryption (~magic3) after encryption.
 * Configurations are inherited from framework.cpp.
 * The red-black tree is implemented in ransomware_set/utils/core/rbtree.h and is migrated from linux kernel's
 * rbtree implementation to user space.
 * @dev-note : in bio.h set CH_DEBUG_TYPE to adjust debug level
 * @version 0.1
 * @date 2023-05-17
 */
#include "bio.h"
#include <unistd.h>
#include <algorithm>
static inline cache_entry_t* get_ceh(struct rb_node* node) {
    return container_of(node, cache_entry_t, node);
}

static inline cache_entry_t* buf_alloc(uint64_t l, uint64_t r, int encrypted, int unencrypted) {
    cache_entry_t* ret;
    try {
        ret = new cache_entry_t;
    } catch (std::exception& e) {
        std::cerr << "Failed to allocate memory for cache_entry_t: " << e.what() << std::endl;
        return NULL;
    }
    ret->l = l;
    ret->r = r;
    ret->is_cache = 1;
    ret->encrypted = encrypted;
    ret->unencrypted = unencrypted;
    return ret;
}

void BIO_Cache::_cache_insert(cache_entry_t* ceh) {
    this->num_entries++;
    this->cached_list.push(ceh->l);
    this->encrypted_len += ceh->encrypted;
    this->unencrypted_len += ceh->unencrypted;
}

int64_t BIO_Cache::rb_tree_alloc_and_insert(struct rb_root* root, uint64_t l, struct rb_node** node, 
uint64_t size) {
    struct rb_node** _new;
    struct rb_node* parent = NULL;
    int64_t ret = -1;
    #ifndef V1_ENABLE
    if(size != 1) { // current implementation is, each time we insert one sector, and we don't merge nodes
        printf("size != 1 : check code version\n");
        return -1;
    }
    #endif

    if(NULL == root) return -1;

    _new = &(root->rb_node);

    while(*_new) {
        cache_entry_t* ceh = get_ceh(*_new);
        parent = *_new;
        if(l < ceh->l) {
            _new = &((*_new)->rb_left);
        } else if(l > ceh->l) {
            _new = &((*_new)->rb_right);
        } else {
            this->debug.add_hit();
            if(ceh->is_cache == 0) {
                ceh->is_cache = 1;
                this->cached_list.push(ceh->l);
                this->num_cached++;
            } else {
                this->debug.hit_empty();
            }
            return 0;
        }
    }

    if(NULL == *_new) {
        cache_entry_t* ceh = buf_alloc(l, l + size - 1, 0, 0);
        this->num_cached++;
        // add it in the rb tree
        rb_link_node(&(ceh->node), parent, _new);
        rb_insert_color(&(ceh->node), root);
        // add the li to the list
        _cache_insert(ceh);
        *node = &(ceh->node);
        ret = 0;
    } else {
        CH_debug("_new != NULL?\n");
        ret = -1;
    }
    #ifdef V1_ENABLE
    merge_rb_node(root, *node);
    #endif
    return ret;
}



/**
 * @brief cache the BIO operation from sector number @param l to @param r (inclusive) by inserting the
 * interval into rb tree by  left bound @param l, in the insertion, merge will be completed and cached list
 * would be changed correspondingly
 * @return int64_t 
 */
int64_t BIO_Cache::cache(uint64_t l, uint64_t r) {
    uint64_t i;
    struct rb_node* node;
    for(i = l; i <= r; i++) {
        if(0 > rb_tree_alloc_and_insert(&(this->cache_root), i, &node, 1)) {
            CH_debug("rb_tree_alloc_and_insert failed\n");
            return -1;
        }
    }
    return 0;
}
