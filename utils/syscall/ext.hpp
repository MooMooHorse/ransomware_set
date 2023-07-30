#ifndef _EXT_HPP_
#define _EXT_HPP_


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
#include <sys/stat.h>

#include "config.hpp"


/**
 * @side-effect : this function assumes the backing device is /dev/vdb
 * @todo : fix this
*/
int ext_get_blocsk(char** filepaths, int num_files, uint64_t* blocks, int max_blocks) {
    int j = 0; // cnt for blocks
    for(int i = 0; i < num_files; i++) {
        char* filepath = filepaths[i];
        if(!file_exits(filepath)) {
            continue;
        }
        struct stat fileStat;
        if(stat(filepath, &fileStat) < 0) {
            continue;
        }
        uint64_t inode = fileStat.st_ino;
        // use debugfs to get blocks
        std::string cmd = "sudo debugfs -R \"blocks <"  + std::to_string(inode) + ">\" /dev/vdb 2>/dev/null";
        FILE* fp = popen(cmd.c_str(), "r");
        char line[100];
        std::string text;
        while (fgets(line, sizeof(line), fp)) {
            // add line to text
            text += std::string(line);
        }
        // the text will be block numbers separated by spaces
        // we extract each block number and put it into blocks
        std::istringstream iss(text);
        std::string block;
        while(iss >> block) {
            blocks[j++] = std::stoi(block);
            if(j == max_blocks) {
                pclose(fp);
                std::cout << "Too many blocks" << std::endl;
                break;
            }
        }
        pclose(fp);

    }
    return j;
}

#endif 