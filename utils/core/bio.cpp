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

static inline cache_entry_t* buf_alloc(uint64_t l, int encrypted, int unencrypted) {
    cache_entry_t* ret;
    try {
        ret = new cache_entry_t;
    } catch (std::exception& e) {
        std::cerr << "Failed to allocate memory for cache_entry_t: " << e.what() << std::endl;
        return NULL;
    }
    ret->l = l;
    ret->encrypted = encrypted;
    ret->unencrypted = unencrypted;
    return ret;
}

void BIO_Cache::_cache_insert(cache_entry_t* ceh) {
    this->num_entries++;
    this->unencrypted_len += ceh->unencrypted;
}

int BIO_Cache::is_rans_on() {
    return this->rans_on;
}

/**
 * @note : if ransomware isn't turned on in framework.cpp, 
 * then we only put encrypted blocks into rb tree.
*/
int64_t BIO_Cache::rb_tree_alloc_and_insert(struct rb_root* root, uint64_t lsa, struct rb_node** node) {
    struct rb_node** _new;
    struct rb_node* parent = NULL;
    int64_t ret = -1;

    if(NULL == root) return -1;

    _new = &(root->rb_node);

    while(*_new) {
        cache_entry_t* ceh = get_ceh(*_new);
        parent = *_new;
        if(lsa < ceh->l) {
            _new = &((*_new)->rb_left);
        } else if(lsa > ceh->l) {
            _new = &((*_new)->rb_right);
        } else {
            if(is_rans_on() && ceh->unencrypted) {
                ceh->encrypted = 1;
                ceh->unencrypted = 0;
                this->unencrypted_len--;
            }
            return 0;
        }
    }
    if(is_rans_on()) { // if ransomware is on, we only need to check how many blocks are corrupted
        return 0;
    }
    if(NULL == *_new) {
        cache_entry_t* ceh = buf_alloc(lsa, 0, 1);
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

void BIO_Cache::dump_clr_blks(const std::string& path) {
    std::ofstream file(path, std::ios::app);
    if(!file.is_open()) {
        CH_debug("Failed to open file %s\n", path.c_str());
        return;
    }
    file << "clean blocks :\n";
    struct rb_node* node;
    for(node = rb_first(&(this->cache_root)); node; node = rb_next(node)) {
        cache_entry_t* ceh = get_ceh(node);
        if(ceh->unencrypted)
            file << ceh->l << " ";
    }
    file << std::endl;
    file << "corrupted blocks :\n";
    for(node = rb_first(&(this->cache_root)); node; node = rb_next(node)) {
        cache_entry_t* ceh = get_ceh(node);
        if(ceh->encrypted)
            file << ceh->l << " ";
    }
    file << std::endl;
    file.close();
}

void BIO_Cache::turn_on_rans() {
    this->rans_on = 1;
}

void BIO_Cache::turn_off_rans() {
    this->rans_on = 0;
}

int BIO_Cache::get_unencrypted() {
    return this->unencrypted_len;
}



/**
 * @brief cache the BIO operation with sector number @param lsa by inserting the
 * interval into rb tree by  left bound @param l, in the insertion, merge will be completed and cached list
 * would be changed correspondingly
 * @return int64_t 
 */
int64_t BIO_Cache::cache(uint64_t lsa) {
    struct rb_node* node;
    if(0 > rb_tree_alloc_and_insert(&(this->cache_root), lsa, &node)) {
        CH_debug("rb_tree_alloc_and_insert failed\n");
        return -1;
    }
    return 0;
}
