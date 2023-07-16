#ifndef _F2FS_H_
#define _F2FS_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstdio>
#include <string>
#include <algorithm>
#include <regex>

#include "config.hpp"

// call dump.f2fs /dev/vdb -b <block_num> 
// check the line that contains "File name"
// a typical line is like:
//  - File name         : F303.roo
// the file name is F303.roo
// so we need to extract the file name from the line
void dump_blocks_f2fs(const uint64_t* blocks, const int num_blocks, const char** filenames, int num_files) {
    uint64_t last_block = 0;
    for (int i = 0; i < num_blocks; i++) {
        if(i%100 == 0) {
            std::cout << "progress: " << i << "/" << num_blocks << std::endl;
        }
        if((blocks[i] >> 3) == (last_block>>3)){
            // printf("%lu ", blocks[i]);
            continue;
        }
        char cmd[100];
        sprintf(cmd, "sudo dump.f2fs /dev/vdb -b %lu", blocks[i] >> 3);
        // std::cout << cmd << std::endl;
        FILE* fp = popen(cmd, "r");
        char line[4096];
        while (fgets(line, sizeof(line), fp)) {
            // std::cout << line;
            if (strstr(line, "File name")) {
                char* p = strstr(line, ":");
                p++;
                while (*p == ' ') p++;
                int len = strlen(p);
                if (p[len - 1] == '\n') p[len - 1] = '\0';
                // strip the white space at the end
                while (p[len - 1] == ' ') {
                    p[len - 1] = '\0';
                    len--;
                }
                // std::cout << p << std::endl;
                // check if p is in filenames
                int j;
                for (j = 0; j < num_files; j++) {
                    if (strcmp(p, filenames[j]) == 0) {
                        break;
                    }
                }
                if(j != num_files) {
                    last_block = blocks[i];
                    // printf("%lu ", blocks[i]);
                } else{
                    printf("%lu ", blocks[i]);
                }
                break;
            }
        }
        pclose(fp);
    }
}



#endif