#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstdio>
#include <string>
#include <algorithm>
#include <regex>

#include "sys.h"

void dbg_dump_filenames(char** filenames, int limit) {
    for(int i = 0; i < limit; i++) {
        std::cout << filenames[i] << std::endl;
    }
}

// get all test files' names
int get_filenames(char** filenames, int limit) {
    // get the path of this file
    std::string path;
    // read link /proc/self/exe
    char buf[PATH_MAX];
    int len = readlink("/proc/self/exe", buf, sizeof(buf));
    if(len == -1) {
        perror("readlink");
        exit(1);
    }
    buf[len] = '\0';
    path = buf;
    // use 
    int path_len = path.length();
    while(path[path_len] != '/' && path_len) path_len--; // bin
    path_len--;
    while(path[path_len] != '/' && path_len) path_len--; // syscall
    path_len--;
    while(path[path_len] != '/' && path_len) path_len--; // utils
    path_len--;
    while(path[path_len] != '/' && path_len) path_len--; // ransomware_set
    if(!path_len) {
        perror("Illegal binary path");
        exit(1);
    }
    std::string config_path;
    // concat the path of this file with config.py
    config_path = path.substr(0, path_len) + "/config.py";

    // "config.py -getfilenames" will print out a set of file names

    std::string cmd = "python3 " + config_path + " -getfilenames";
    FILE* fp = popen(cmd.c_str(), "r");
    char line[100];
    int i = 0;
    while (fgets(line, sizeof(line), fp)) {
        int len = strlen(line);
        if (line[len - 1] == '\n') line[len - 1] = '\0';
        // strip the white space at the end
        while (line[len - 1] == ' ') {
            line[len - 1] = '\0';
            len--;
        }
        filenames[i] = (char*)malloc(sizeof(char) * (len + 1));
        strcpy(filenames[i], line);
        i++;
        if(i == limit){
            perror("Too many files");
            break;
        }
    }

    pclose(fp);
    return i;
}

// get all test files' paths
int get_paths(char** fpaths, int limit) {
    // get the path of this file
    std::string path;
    // read link /proc/self/exe
    char buf[PATH_MAX];
    int len = readlink("/proc/self/exe", buf, sizeof(buf));
    if(len == -1) {
        perror("readlink");
        exit(1);
    }
    buf[len] = '\0';
    path = buf;
    // use 
    int path_len = path.length();
    while(path[path_len] != '/' && path_len) path_len--; // bin
    path_len--;
    while(path[path_len] != '/' && path_len) path_len--; // syscall
    path_len--;
    while(path[path_len] != '/' && path_len) path_len--; // utils
    path_len--;
    while(path[path_len] != '/' && path_len) path_len--; // ransomware_set
    if(!path_len) {
        perror("Illegal binary path");
        exit(1);
    }
    std::string config_path;
    // concat the path of this file with config.py
    config_path = path.substr(0, path_len) + "/config.py";

    // "config.py -getfilepaths" will print out a set of file paths

    std::string cmd = "python3 " + config_path + " -getfilepaths";
    FILE* fp = popen(cmd.c_str(), "r");
    char line[100];
    int i = 0;
    while (fgets(line, sizeof(line), fp)) {
        int len = strlen(line);
        if (line[len - 1] == '\n') line[len - 1] = '\0';
        // strip the white space at the end
        while (line[len - 1] == ' ') {
            line[len - 1] = '\0';
            len--;
        }
        fpaths[i] = (char*)malloc(sizeof(char) * (len + 1));
        strcpy(fpaths[i], line);
        i++;
        if(i == limit){
            perror("Too many files");
            break;
        }
    }

    pclose(fp);
    return i;
}

// get all blocks of the files
int get_blocks(uint64_t* blocks, int limit) {
    diag_ctrl_t ctrl = {MAGIC3 , 0, {NULL}, 0, NULL, 0};
    ctrl.blk2file_size = limit;
    ctrl.blk2file = (uint64_t*)malloc(sizeof(uint64_t) * limit);
    ctrl.blk2file_size = call(GET_MAP, &ctrl);
    for(int i = 0; i < ctrl.blk2file_size; i++) {
        blocks[i] = ctrl.blk2file[i];
    }
    // std::cout << "blk2file_size: " << ctrl.blk2file_size << std::endl;
    return ctrl.blk2file_size;
}

#endif 