#ifndef _BIO_H_
#define _BIO_H_
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include "rbtree.h"
#include <queue>
#include <unordered_map>

#define SEC_TO_BYTES(sec)  ((sec) << 9)
#define SEC_SIZE           512

#define NUM2CHAR(num)      ((char)(num&0xff))

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

typedef enum ENCODING_TYPE {
    UTF8 = 0,
    UTF16,
} encoding_t;


typedef struct CACHE_ENTRY {
    uint64_t l, r; // l, r should be sector size aligned
    int is_cache; // if this is a cached BIO (lazy update)
    int encrypted, unencrypted; // if not lazy update, encrypted and unencrypted should reflect the 
    // real number of bytes encrypted and unencrypted in a sector
    struct rb_node node;
} cache_entry_t;

typedef struct RECORD_DBG_FILE {
    uint64_t sec_off;
    uint64_t len;
    int dye_color;
} record_dbg_file_t;





class BIO_Cache {
private:
    uint64_t unencrypted_len, encrypted_len;
    int num_entries;
    uint64_t magic1, magic2, magic3;
    int min_len;
    struct rb_root cache_root;
public:
    BIO_Cache(uint64_t unencrpted_len, uint64_t encrypted_len, 
    uint64_t files_recoverable, uint64_t files_unrecoverable,
    int num_entries, int num_cached,
    uint64_t magic1, uint64_t magic2, uint64_t magic3, 
    ) {
        this->unencrypted_len = unencrpted_len;
        this->encrypted_len = encrypted_len;
        this->num_entries = num_entries;
        this->magic1 = magic1;
        this->magic2 = magic2;
        this->magic3 = magic3;
        this->cache_root = RB_ROOT;
        this->min_len = 4; // only 4 or 8
    }
    
    int64_t cache(uint64_t l, uint64_t r);
private:
    int64_t rb_tree_alloc_and_insert(struct rb_root* root, uint64_t l, struct rb_node** node, 
    uint64_t size);
};

#endif /* _BIO_H_ */