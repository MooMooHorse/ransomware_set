#ifndef _BIO_H_
#define _BIO_H_
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include "rbtree.h"
#include <queue>

#define SEC_TO_BYTES(sec)  ((sec) << 9)
#define SEC_SIZE           512

#define ENCRYPT(byte)      (~(byte))

// debug level
#define CH_NO_DEBUG            0
#define CH_BASIC_DEBUG         (1<<0)
#define CH_CACHE_DEBUG         (1<<1)    // used for BIO caching related
#define CH_FLUSH_DEBUG         (1<<2)    // used for BIO flushing related
#define CH_DEBUG_TYPE          (CH_BASIC_DEBUG)

// define our debug printk : this does nothing when we're at particular debug level
#if (CH_DEBUG_TYPE != CH_NO_DEBUG)
#define CH_debug(fmt, args...) \
    printf(fmt, ##args)
#else
#define CH_debug(fmt, args...)
#endif

// BIO Cache types
#define PRE_ATTACK          0
#define POST_ATTACK         1


typedef struct CACHE_ENTRY {
    uint64_t l, r; // l, r should be sector size aligned
    int is_cache; // if this is a cached BIO (lazy update)
    int encrypted, unencrypted; // if not lazy update, encrypted and unencrypted should reflect the 
    // real number of bytes encrypted and unencrypted in a sector
    struct rb_node node;
} cache_entry_t;

class BIO_debug {
private:
    int BIO_hit; // how many times BIO cache is hit
    int BIO_hit_empty; // how many times BIO cache is hit but original cache entry does not contain related info
    int BIO_hit_encrpted, BIO_hit_unencrypted; // how many encrypted and unencrypted bytes are overwritten by BIO hit
    int rans_en; // if ransomware has been enabled
    int rans_overwrite; // how many unencrypted bytes are overwritten by ranswomare at flush time
    int snapshot_encrypted, snapshot_unencrypted; // how many encrypted and unencrypted bytes are overwritten by snapshot
public:
    BIO_debug() : BIO_hit(0), BIO_hit_empty(0), BIO_hit_encrpted(0),
     BIO_hit_unencrypted(0), rans_en(0), rans_overwrite(0) {}
    void clear() {
        this->BIO_hit = 0;
        this->BIO_hit_empty = 0;
        this->BIO_hit_encrpted = 0;
        this->BIO_hit_unencrypted = 0;
        this->rans_en = 0;
        this->rans_overwrite = 0;
    }
    void enable_rans() {
        this->rans_en = 1;
    }
    void rans_overwrite_bytes(int bytes) {
        this->rans_overwrite += bytes;
    }
    void add_hit() {
        this->BIO_hit++;
    }
    void hit_empty() {
        this->BIO_hit_empty++;
    }
    void hit_encrypted(int len) {
        this->BIO_hit_encrpted += len;
    }
    void hit_unencrypted(int len) {
        this->BIO_hit_unencrypted += len;
    }
    void snapshot(int64_t encrypted, int64_t unencrypted) {
        this->snapshot_encrypted = encrypted;
        this->snapshot_unencrypted = unencrypted;
    }
    void dump_rans() {
        if(this->rans_en){
            std::cout << "rans_en: " << this->rans_en << std::endl;
            std::cout << "rans_overwrite: " << this->rans_overwrite << std::endl;
            std::cout << "bio hit " << this->BIO_hit << std::endl;
            std::cout << "bio hit empty " << this->BIO_hit_empty << std::endl;
            std::cout << "encrypted hit length (bytes) " << this->BIO_hit_encrpted << std::endl;
            std::cout << "unencrypted hit length (bytes) " << this->BIO_hit_unencrypted << std::endl;
        }
    }
    void dump_snapshot() {
        std::cout << "snapshot_encrypted snapshot_unencrypted" << std::endl;
        std::cout << this->snapshot_encrypted << " " << this->snapshot_unencrypted << std::endl;
    }

};

class BIO_Cache {
private:
    uint64_t unencrypted_len, encrypted_len;
    int num_entries, num_cached;
    uint64_t magic1, magic2, magic3;
    int min_len;
    struct rb_root cache_root;
    std::priority_queue<int, std::vector<int>, std::greater<int>> cached_list;
    std::string deviceName;
public:
    BIO_Cache(uint64_t unencrpted_len, uint64_t encrypted_len, int num_entries, int num_cached,
    uint64_t magic1, uint64_t magic2, uint64_t magic3, 
    const std::string& deviceName) {
        this->unencrypted_len = unencrpted_len;
        this->encrypted_len = encrypted_len;
        this->num_entries = num_entries;
        this->num_cached = num_cached;
        this->magic1 = magic1;
        this->magic2 = magic2;
        this->magic3 = magic3;
        this->cache_root = RB_ROOT;
        this->deviceName = deviceName;
        this->min_len = 4; // only 4 or 8
    }
    
    int64_t cache(uint64_t l, uint64_t r);
    void flush();
    void report();
    void snapshot();

    // debug
    BIO_debug debug;
    int sanity_check();
private:
    int64_t rb_tree_alloc_and_insert(struct rb_root* root, uint64_t l, struct rb_node** node, 
    uint64_t size);
    int get_encrpted(const std::vector<char>& buf, uint64_t sec_off);
    int get_unencrpted(const std::vector<char>& buf, uint64_t sec_off);
    // update metadata when inserting, merging or removing
    void _cache_insert(cache_entry_t* ceh);
    void _cache_remove(cache_entry_t* ceh);
#ifdef V1_ENABLE
    int64_t merge_rb_node(struct rb_root* root, struct rb_node* node);
#endif
};

#endif /* _BIO_H_ */