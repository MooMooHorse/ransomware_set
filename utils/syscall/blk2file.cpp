#include "sys.h"
#include "f2fs.hpp"
#include "ext.hpp"
#include "btrfs.hpp"
#include "xfs.hpp"
#include "ntfs.hpp"

#define MAX_BLCOKS 400000
#define MAX_FILES 40000
#define MAX_FNAME_LEN 32

int main() {
    uint64_t blocks[MAX_BLCOKS];
    char** filenames;
    filenames = (char**)malloc(sizeof(char*) * MAX_FILES);
    for(int i = 0; i < MAX_FILES; i++) {
        filenames[i] = (char*)malloc(sizeof(char) * MAX_FNAME_LEN);
    }
    int num_blocks = get_blocks(blocks, MAX_BLCOKS);
    int num_files = get_filenames(filenames, MAX_FILES);
    std::cout << "num_blocks: " << num_blocks << " " << "num_files: " << num_files << std::endl;
    dump_blocks_f2fs(blocks, num_blocks, (const char**)filenames, num_files);
    return 0;
}