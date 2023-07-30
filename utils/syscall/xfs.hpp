#ifndef XFS_HPP_H
#define XFS_HPP_H


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



int xfs_get_blocks(char** filepaths, int num_files, uint64_t* blocks, int max_blocks) {
    int j = 0; // cnt for blocks
    for(int i = 0; i < num_files; i++) {
        // call sudo xfs_map filepaths[i] to get text output
        // parse the text output to get blocks
        if(!file_exits(filepaths[i])) {
            continue;
        }
        std::string cmd = "sudo xfs_bmap " + std::string(filepaths[i]);
        FILE* fp = popen(cmd.c_str(), "r");
        char line[100];
        std::string text;
        while (fgets(line, sizeof(line), fp)) {
            // add line to text
            text += std::string(line);
        }
        // text="0: [0..119]: 26657344..26657463";
        // use regex to match this pattern extracting the last two numbers
        std::regex rgx("\\[[0-9]+\\.\\.[0-9]+\\]: ([0-9]+)\\.\\.([0-9]+)");

        

        std::smatch match;
        // if there is a match
        if(std::regex_search(text, match, rgx)) {
            for(int k = std::stoi(match[1].str()); k <= stoi(match[2].str()); k++) {
                blocks[j++] = k;
                if (j == max_blocks) {
                    pclose(fp);
                    std::cout << "Too many blocks" << std::endl;
                    break;
                }
            }
        }
        else {
            continue;
        }
        pclose(fp);
    }
    return j;
}


#endif