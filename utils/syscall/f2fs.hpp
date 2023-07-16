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
void dump_blocks_f2fs(const uint64_t* blocks, const int num, const char** filenames) {
    for (int i = 0; i < num; i++) {
        char cmd[100];
        sprintf(cmd, "dump.f2fs /dev/vdb -b %lu", blocks[i]);
        FILE* fp = popen(cmd, "r");
        char line[100];
        while (fgets(line, sizeof(line), fp)) {
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
                // check if p is in filenames
                int j;
                for (j = 0; j < num; j++) {
                    if (strcmp(p, filenames[j]) == 0) {
                        break;
                    }
                }
                if(j != num) {
                    printf("%lu ", blocks[i]);
                }
                break;
            }
        }
        pclose(fp);
    }
}



#endif