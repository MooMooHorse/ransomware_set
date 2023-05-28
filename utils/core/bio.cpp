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
                this->debug.hit_encrypted(ceh->encrypted);
                this->debug.hit_unencrypted(ceh->unencrypted);
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

#ifdef ENABLE_V1
// this code will not be able to execute under user privilege
static void readDataIntoVector(std::vector<char>& data, const std::string& inputFileName, 
int secNum, int sectorCount) {
    std::ifstream input(inputFileName, std::ios::binary);  // Open input file in binary mode

    if (!input) {
        std::cerr << "Failed to open input file." << std::endl;
        return;
    }

    // Seek to the desired position
    input.seekg(secNum * SEC_SIZE, std::ios::beg);

    // Read the data into the vector
    input.read(data.data(), sectorCount * SEC_SIZE);

    input.close();
}
#endif

static void readDataIntoVector(std::vector<char>& data, const std::string& inputFileName, 
int secNum, int sectorCount) {
    std::string command = "sudo dd if=" + inputFileName + " skip=" + std::to_string(secNum)
                          + " count=" + std::to_string(sectorCount) + " status=none";

    // Open a pipe to execute the dd command
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run dd command" << std::endl;
        return;
    }

    // Read the data into the vector
    if ((size_t)sectorCount !=
    fread(data.data(), SEC_SIZE, sectorCount, pipe)) {
        std::cerr << "Failed to read data from pipe" << std::endl;
        return;
    }

    pclose(pipe);
}

static struct rb_node* rb_search(struct rb_root* root, uint64_t l) {
    struct rb_node* node = root->rb_node;
    while(node) {
        cache_entry_t* ceh = get_ceh(node);
        if(l < ceh->l) {
            node = node->rb_left;
        } else if(l > ceh->l) {
            node = node->rb_right;
        } else {
            return node;
        }
    }
    return NULL;
}



int BIO_Cache::get_encrpted(const std::vector<char>& buf, uint64_t sec_off) {
    // iterate buf starting from SEC_TO_BYTES(sec_off), count the number of bytes == 255 - this->magic_3
    // and return the count
    int ret = 0;
    uint32_t i;
    uint64_t big_magic, target_magic;
    uint64_t _magic1, _magic2, _magic3, _magic4;
    target_magic = (ENCRYPT(this->magic3) << 24) | (ENCRYPT(this->magic3) << 16) | (ENCRYPT(this->magic3) << 8) | ENCRYPT(this->magic3);

    for(i = SEC_TO_BYTES(sec_off); i < SEC_TO_BYTES(sec_off) + SEC_SIZE; i += this->min_len) {
        _magic1 = buf[i]; _magic2 = buf[i + 1]; _magic3 = buf[i + 2]; _magic4 = buf[i + 3];
        big_magic = (_magic1 << 24) | (_magic2 << 16) | (_magic3 << 8) | _magic4;
        ret += (big_magic == target_magic) * this->min_len;
    }
    return ret;
}

int BIO_Cache::get_unencrpted(const std::vector<char>& buf, uint64_t sec_off) {
    // iterate buf starting from SEC_TO_BYTES(sec_off), count the number of bytes == this->magic_3
    // and return the count
    int ret = 0;
    uint32_t i;
    uint64_t big_magic, target_magic;
    uint64_t _magic1, _magic2, _magic3, _magic4;
    target_magic = ((this->magic3) << 24) | ((this->magic3) << 16) | ((this->magic3) << 8) | (this->magic3);

    for(i = SEC_TO_BYTES(sec_off); i < SEC_TO_BYTES(sec_off) + SEC_SIZE; i += this->min_len) {
        _magic1 = buf[i]; _magic2 = buf[i + 1]; _magic3 = buf[i + 2]; _magic4 = buf[i + 3];
        big_magic = (_magic1 << 24) | (_magic2 << 16) | (_magic3 << 8) | _magic4;
        ret += (big_magic == target_magic) * this->min_len;
    }
    return ret;
}

/**
 * @brief flush the current BIO cache by removing all the entries in the cached list and clear the is_cache
 * flag in the rb tree, we update each node's encrypted and unencrypted bytes in the cached list
 */
void BIO_Cache::flush() {
    printf("flushing all the requests...\n");
    uint64_t total_sectors = this->cached_list.size();
    uint64_t cur_sectors = 0;
    while(!this->cached_list.empty()) {
        // iterate through the priority queue until the first element that's not continuous to the previous one
        // and remove all the elements in the cached list
        int prev = -1;
        int cur = this->cached_list.top();
        uint64_t l,r;
        l = cur;
        while(!this->cached_list.empty() && (prev < 0 || prev + 1 == cur)) {
            prev = cur;
            this->cached_list.pop();
            cur = this->cached_list.top();
        }
        r = prev;
        if(prev < 0) {
            CH_debug("prev < 0?\n");
            return;
        }
        std::vector<char> data(SEC_TO_BYTES(r - l + 1));
        readDataIntoVector(data, this->deviceName, l, r - l + 1);

        // for each sector(node on rb tree), we find the node, chanage if_cache
        // and update the encrypted and unencrypted bytes
        struct rb_node* node, *i;
        if(NULL == (node = rb_search(&(this->cache_root), l))) {
            CH_debug("rb_search failed\n");
            return;
        }
        for(i = node; i && get_ceh(i)->l <= r; i = rb_next(i)) {
            cache_entry_t* ceh = get_ceh(i);
            ceh->is_cache = 0;
            this->num_cached--;
            this->encrypted_len -= ceh->encrypted;
            this->unencrypted_len -= ceh->unencrypted;
            this->debug.rans_overwrite_bytes(ceh->unencrypted);
            ceh->encrypted = get_encrpted(data, ceh->l - l);
            ceh->unencrypted = get_unencrpted(data, ceh->l - l);
            this->encrypted_len += ceh->encrypted;
            this->unencrypted_len += ceh->unencrypted;
        }
        // update status bar
        if( (cur_sectors + r - l + 1) * 100 / total_sectors - (cur_sectors) * 100 / total_sectors > 0) {
            std::string status = "[";
            for(uint64_t i = 0; i < (cur_sectors + r - l + 1) * 10 / total_sectors; i++) {
                status += "=";
            }
            status += ">";
            // pad to 10
            for(uint64_t i = (cur_sectors + r - l + 1) * 10 / total_sectors + 1; i < 10; i++) {
                status += " ";
            }
            status += "]";

            std::cout << "flushed " << " " << status << " " << (cur_sectors + r - l + 1) * 100 / total_sectors << "%\r" << std::flush;
        }
        cur_sectors += (r - l + 1);

    }
    std::cout << "............All flushed............" << std::endl;
    this->debug.dump_rans();
}


void BIO_Cache::get_recoverable(uint64_t& recoverable, uint64_t& unrecoverable, std::vector<std::string>& unencrypted_files, 
                            const std::vector<char>& buf, uint64_t sec_off) {
    uint64_t _magic1, _magic2, _magic3, _magic4;
    uint64_t big_magic;
    std::string file_name;
    uint32_t i;
    int fname_start = 0;
    for(i = SEC_TO_BYTES(sec_off); i + 3 < SEC_TO_BYTES(sec_off) + SEC_SIZE; i++) {
        _magic1 = (uint8_t)buf[i]; _magic2 = (uint8_t)buf[i + 1]; _magic3 = (uint8_t)buf[i + 2]; _magic4 = (uint8_t)buf[i + 3];
        big_magic = (_magic1 << 24) | (_magic2 << 16) | (_magic3 << 8) | _magic4;
        if(big_magic == this->magic1){
            fname_start = 1;
            i += 3;
        
        }else if(big_magic == this->magic2 || big_magic == ENCRYPT(this->magic2)){
            if(big_magic == this->magic2 && fname_start) {
                unencrypted_files.push_back(file_name);
                recoverable ++;
            } else {
                unrecoverable ++;
            }
            fname_start = 0;
            file_name = "";
        } else if(fname_start) {
            file_name += buf[i];
        }
    }
}

/**
 * @brief snapshot function behvaes like flush, except that it does not change is_cache property
 * and it does not care about if any entries is cached or not.
 * It simply flushed every entry in RB tree and it does not update the encrypted and unencrypted bytes.
 * It also does NOT have any side effect on the RB tree itself.
 * Instead the encrtyped data and unencrypted data will be updated to the data pointed by @param encrypted_len
 * @param unencrypted_len.
 * @version 2.0 update : add support for recoverable files.
*/
void BIO_Cache::snapshot() {
    int64_t _encrypted_len = 0, _unencrypted_len = 0;

    this->files_recoverable = this->files_unrecoverable = 0;
    this->unencrypted_files.clear();

    // iterate through rb tree.
    struct rb_node* node;
    node = rb_first(&(this->cache_root));
    while(node) {
        struct rb_node *lnode, *rnode;
        struct rb_node *prev, *cur; 
        uint64_t l, r;
        // we iterate through rb tree to find a continuous interval
        lnode = prev = node;
        for(cur = rb_next(prev); cur && get_ceh(cur)->l == get_ceh(prev)->l + 1; cur = rb_next(prev)) {
            prev = cur;
        }
        rnode = prev;
        l = get_ceh(lnode)->l;
        r = get_ceh(rnode)->l;

        std::vector<char> data(SEC_TO_BYTES(r - l + 1));
        readDataIntoVector(data, this->deviceName, l, r - l + 1);

        // for each sector(node on rb tree), we find the node, chanage if_cache
        // and update the encrypted and unencrypted bytes
        struct rb_node* _node, *i;
        if(NULL == (_node = rb_search(&(this->cache_root), l))) {
            CH_debug("rb_search failed\n");
            return;
        }
        for(i = _node; i && get_ceh(i)->l <= r; i = rb_next(i)) {
            cache_entry_t* ceh = get_ceh(i);
            _encrypted_len += get_encrpted(data, ceh->l - l);
            _unencrypted_len += get_unencrpted(data, ceh->l - l);
            this->get_recoverable(this->files_recoverable, this->files_unrecoverable, this->unencrypted_files,
             data, ceh->l - l);
        }
        node = rb_next(rnode);
    }
    this->debug.snapshot(_encrypted_len, _unencrypted_len);
}

/**
 * @brief Check if the current BIO cache makes sense by checking if there are overlapping entries
 * and if the sum of all the entries equals to unencrypted_len
 * @return int 
 */
int BIO_Cache::sanity_check() {
    // first check if there are overlapping entries
    struct rb_node* node;
    uint64_t encrpted_sum = 0, unencrypted_sum = 0;
    int32_t count = 0;

    if(this->min_len != 4) {
        printf("min_len != 4 : bad bad :( other cases not covered\n");
        return -1;
    }
    for(node = rb_first(&(this->cache_root)); node; node = rb_next(node)) {
        cache_entry_t* ceh = get_ceh(node);
        encrpted_sum += ceh->encrypted;
        unencrypted_sum += ceh->unencrypted;
        count++;
    }
    if(count != this->num_entries) {
        CH_debug("BIO cache sanity check failed: count != num_entries\n");
        return -1;
    }
    if(this->num_cached != (int)this->cached_list.size()) {
        CH_debug("BIO cache sanity check failed: num_cached != cached_list.size()\n");
        return -1;
    }

    return encrpted_sum == this->encrypted_len && unencrypted_sum == this->unencrypted_len;
}

// debugging function that prints out the current environment for tar_sys_info
static void dump_tar_sys_info_env() {
    // Print the current directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << "Current directory: " << cwd << std::endl;
    } else {
        std::cerr << "Error: could not get current directory" << std::endl;
    }
    // execute a ls in rans_test/tar_sys_info
    std::string cmd = "ls -l rans_test/tar_sys_info";
    std::cout << "Executing: " << cmd << std::endl;
    system(cmd.c_str());
}

void BIO_Cache::snapshot_report(const std::string& dumpFilePath) {
    std::unordered_map<std::string, std::size_t> file_sizes;

    std::cout << "----BIO Cache Snapshot Report----" << std::endl;

    printf("dentries recoverable : %lu\n", this->files_recoverable);
    printf("dentries unrecoverable : %lu\n", this->files_unrecoverable);
    // printf("unencrypted files : \n");
    // for(uint64_t i = 0; i < this->unencrypted_files.size(); i++) {
    //     printf("%s\n", this->unencrypted_files[i].c_str());
    // }
    // remove the repeat file names
    std::sort(this->unencrypted_files.begin(), this->unencrypted_files.end());
    auto last = std::unique(this->unencrypted_files.begin(), this->unencrypted_files.end());
    this->unencrypted_files.erase(last, this->unencrypted_files.end());
    
    printf("number of unencrypted files : %lu\n", this->unencrypted_files.size());
    // try to find totally recoverable length of files
    // the dump file given by dumpFilePath has the following format
    //<number_of_files> <total_size>
    // <file_path_1> <file_size_1>
    // <file_path_2> <file_size_2>
    // ...
    uint64_t total_size = 0;
    std::ifstream dumpFile;
    dumpFile.open(dumpFilePath);
    if(!dumpFile.is_open()) {
        dump_tar_sys_info_env();
        printf("dump file %s not found\n", dumpFilePath.c_str());
        return;
    }
    int num_files;
    dumpFile >> num_files >> total_size;
    printf("total size of files : %lu\n", total_size);
    for(int i = 0; i < num_files; i++) {
        std::string file_path;
        size_t file_size;
        dumpFile >> file_path >> file_size;
        if(file_size == 0) {
            printf("file %s is empty\n", file_path.c_str());
            continue;
        }
        // file path is a path, we want to extract the file name
        file_path = file_path.substr(file_path.find_last_of("/") + 1);
        file_sizes[file_path] = file_size;
    }
    // iterate through unencrypted_files
    total_size = 0;
    for(uint64_t i = 0; i < this->unencrypted_files.size(); i++) {
        std::string file_name = this->unencrypted_files[i];
        if (file_sizes.count(file_name) > 0) {
            total_size += file_sizes[file_name];
        }
    }
    printf("total size of recoverable files : %lu\n", total_size);

    std::cout << "----BIO Cache Snapshot Report End----" << std::endl;
}


void BIO_Cache::report() {

    printf("------- report start -------\n");
    printf("unencrypted_len : %lu\n", this->unencrypted_len);
    printf("encrypted_len : %lu\n", this->encrypted_len);
    printf("num_entries : %d\n", this->num_entries);
    printf("num_cached : %d\n", this->num_cached);
    printf("------- report end -------\n");
    if(sanity_check() <= 0) {
        printf("BIO cache sanity check failed\n");
        return;
    }
    printf("BIO cache sanity check passed...\n");

}

// below are legacy code (old ideas and implementation)

#ifdef V1_ENABLE

// remove overlapping @param len from corresponding length
void BIO_Cache::_cache_remove(cache_entry_t* ceh) {
    this->num_entries--;
    this->encrypted_len -= ceh->encrypted;
    this->unencrypted_len -= ceh->unencrypted;
    ceh->encrypted = 0;
    ceh->unencrypted = 0;
}

// merge the current node with prev and next node if they overlap
// @note : only merge existing neighboring nodes with the same is_cache status
int64_t BIO_Cache::merge_rb_node(struct rb_root* root, struct rb_node* node) {
    struct rb_node* prev = rb_prev(node);
    struct rb_node* next = rb_next(node);
    cache_entry_t* ceh = get_ceh(node);
    cache_entry_t* ceh_prev = NULL;
    cache_entry_t* ceh_next = NULL;
    int64_t ret = 0;

    if(NULL != prev && ceh->is_cache == get_ceh(prev)->is_cache) {
        ceh_prev = get_ceh(prev);
        if(ceh_prev->r >= ceh->l - 1) {
            _cache_remove(ceh); // remove overlapping bytes
            list_del(&(ceh->li));
            // merge prev and current node
            ceh_prev->r = ceh->r;
            rb_erase(node, root);
            delete ceh;
            node = prev;
            ceh = ceh_prev;
        }
    }

    if(NULL != next && ceh->is_cache == get_ceh(next)->is_cache) {
        ceh_next = get_ceh(next);
        if(ceh_next->l - 1 <= ceh->r) {
            _cache_remove(ceh_next); // remove overlapping bytes
            list_del(&(ceh_next->li));
            // merge next and current node
            ceh->r = ceh_next->r;
            rb_erase(next, root);
            delete ceh_next;
        }
    }
    return ret;
}

#endif 

