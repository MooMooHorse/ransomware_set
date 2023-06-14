#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstdio>
#include <string>
#include <algorithm>
#include <regex>
#include "bio.h"

// the following system call(s) are defined by us
#define TRACE_SYS_NUM           428 // the system call number of trace syscall 



class TestingFramework {
private:
    std:: string traceFilePath;
    std:: vector<std:: string> devices;
    std:: string outputDirPath;
    BIO_Cache* cache;
    uint64_t magic1, magic2, magic3;
    encoding_t encoding;
    int framework_ind;
public:
    TestingFramework(const std::string& traceFile, const std::vector<std::string>& devices, 
    const std::string& outputDir, const uint64_t& m1, const uint64_t& m2, const uint64_t& m3, const encoding_t& encoding, int framework_ind)
        : traceFilePath(traceFile), devices(devices), outputDirPath(outputDir), 
        magic1(m1), magic2(m2), magic3(m3), encoding(encoding), framework_ind(framework_ind) {}
    void printParameters() const {
        // core functionality initialized
        std::cout << "Core Functionality Initialized..." << std::endl;
        std::cout << "Trace File: " << traceFilePath << std::endl;
        std::cout << "Disk Devices: " << std::endl;
        for (const std::string& disk_name : devices) {
            std::cout << disk_name << std::endl;
        }
        std::cout << "Log Directory: " << outputDirPath << std::endl;
        std::cout << "Magic Numbers: " << magic1 << " " << magic2 << " " << magic3 << std::endl;
        if(encoding == UTF8) {
            std::cout << "Encoding: UTF8" << std::endl;
        } else if(encoding == UTF16) {
            std::cout << "Encoding: UTF16" << std::endl;
        } else {
            std::cout << "Encoding: UNKNOWN" << std::endl;
        }
    }


    void run() {
        // run the core functionality
        // std::cout << "Running Core Functionality..." << std::endl;
        // try {
        //     cache = new BIO_Cache(0, 0, 0, 0, 0, 0, magic1, magic2, magic3, this->devices, this->encoding);
        // } catch (std::exception& e) {
        //     std::cerr << "Failed to initialize BIO_Cache: " << e.what() << std::endl;
        //     return;
        // }
        // cache_BIO();
        // this->cache->flush();
        // this->cache->report();
        // this->cache->snapshot();
        // this->cache->snapshot_report(this->tarSysDumpPath);
        // // this->cache->debug.dump_dbg_file();
        
        // if(framework_ind == 0) launch_ransomware();
        // else launch_ransomware_backup();
        
        // this->cache->debug.clear();
        // this->cache->debug.enable_rans();
        // cache_BIO();
        // this->cache->flush();
        // this->cache->report();
        // this->cache->snapshot();
        // this->cache->snapshot_report(this->tarSysDumpPath);
        // this->cache->debug.dump_snapshot();
        // this->cache->debug.dump_dbg_file();
    }
private:
    /**
     * @brief given traceFilePath, read the lines 
     */
    void cache_BIO() {
        uint64_t secNum, secCount;
        std::ifstream file(traceFilePath);
        if (!file) {
            std::cerr << "Failed to open trace file." << std::endl;
            return;
        }
        std::string line;
    #if (CH_DEBUG_TYPE & CH_CACHE_DEBUG)
        // get the number of lines in the file
        uint64_t numLines = 0, curLine = 0, ratio = 0;
        while (std::getline(file, line)) {
            ++numLines;
        }
        printf("Number of lines in the trace file: %lu\n", numLines);
        file.clear();
        file.seekg(0, std::ios::beg);
    #endif
        while (std::getline(file, line)) {
    #if (CH_DEBUG_TYPE & CH_CACHE_DEBUG)
            // if ratio changed, print it to tell the user the progress
            if (curLine * 100 / numLines != ratio) {
                ratio = curLine * 100 / numLines;
                printf("Progress: %lu%%\n", ratio);
            }
            ++curLine;
    #endif
            std::istringstream iss(line);
            std::vector<std::string> tokens;
            std::string token;
            while (iss >> token) {
                tokens.push_back(token);
            }
            tokens.erase(std::remove(tokens.begin(), tokens.end(), ""), tokens.end());
            // if tokens is not with 4 elements exactly (i.e. not a valid line), skip
            if(tokens.size() != 4) continue;
            // if token[3] does not include W, skip
            if(tokens[3].find('W') == std::string::npos) continue;
            secNum = std::stoull(tokens[1]);
            secCount = std::stoull(tokens[2]) / 512;
            if(secCount == 0) continue;
            this->cache->cache(secNum, secNum + secCount - 1);
        }

        file.close();
    }
    // we run python3 utils/preprocess.py -run, and wait for it to complete.
    // the output of this process should be re-directed to a file in the output directory
    void launch_ransomware() {
        // std::string cmd = "python3 utils/preprocess.py -run";
        // char buffer[128];
        // std::string args = "";
        // FILE* pipe = popen(cmd.c_str(), "r");
        // if (!pipe) {
        //     std::cerr << "Failed to run command" << std::endl;
        //     return;
        // }
        // while (std::fgets(buffer, sizeof(buffer), pipe) != NULL) {
        //     args += buffer;
        // }
        // pclose(pipe);

        // // if the output directory does not exist, create it
        // std::string cmd2 = "mkdir -p " + outputDirPath;
        // system(cmd2.c_str());

        // // put the output of the python script into a file
        // std::ofstream file(outputDirPath + "/ransomware_output.txt");
        // file << args;
        // file.close();
        // std::cout << "Ransomware launched." << std::endl;
        // std::cout << "Output file: " << outputDirPath + "/ransomware_output.txt" << std::endl;
    }

    void launch_ransomware_backup() {
        // std::string cmd = "python3 utils/preprocess.py -runbackup";
        // char buffer[128];
        // std::string args = "";
        // FILE* pipe = popen(cmd.c_str(), "r");
        // if (!pipe) {
        //     std::cerr << "Failed to run command" << std::endl;
        //     return;
        // }
        // while (std::fgets(buffer, sizeof(buffer), pipe) != NULL) {
        //     args += buffer;
        // }
        // pclose(pipe);

        // // if the output directory does not exist, create it
        // std::string cmd2 = "mkdir -p " + outputDirPath;
        // system(cmd2.c_str());

        // // put the output of the python script into a file
        // std::ofstream file(outputDirPath + "/backup_ransomware_output.txt");
        // file << args;
        // file.close();
        // std::cout << "Ransomware launched." << std::endl;
        // std::cout << "Output file: " << outputDirPath + "/backup_ransomware_output.txt" << std::endl;
    }

};

/**
 * @brief handle the flags to the program. 
 * Usage : 
 * ./core -trace=1 -id=<id>
 * open the trace for test case <id>
 * ./core -trace=0 -id=<id>
 * close the trace for test case <id>
*/
int handle_args(int argc, char** argv) {
    int traceOn = -1;
    int id;
    if (argc < 3) {
        std::cerr << "Usage: ./core [-trace=0/1] -id=<id> [-clear]`" << std::endl;
        return -1;
    }
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        // check if -trace is specified
        if (arg.find("-trace=") != std::string::npos) {
            std::string trace = arg.substr(arg.find("=") + 1);
            if (trace == "1") {
                traceOn = 1;
            } else if (trace == "0") {
                traceOn = 0;
            } else {
                std::cerr << "Usage: ./core -trace=0/1" << std::endl;
                return -1;
            }
        } else if(arg.find("-id=") != std::string::npos) {
            std::string _id = arg.substr(arg.find("=") + 1);
            if (_id == "") {
                std::cerr << "Usage: ./core -id=<id>" << std::endl;
                return -1;
            }
            id = std::stoi(_id);
        } else if(arg.find("-clear")) {
            // make system call to clear the trace
            return 0;
        } else {
            std::cerr << "Usage: ./core -trace=0/1" << std::endl;
            return -1;
        }
    }
    if(traceOn != -1) {
        // make system call to enable / disable trace
        if(traceOn == 1) {
            printf("Trace enabled for id %d\n", id);
        } else {
            printf("Trace disabled for id %d\n", id);
        }
    }
    return 0;
}


int main(int argc, char** argv) {
    handle_args(argc, argv);
    std::string cmd = "python3 config.py -tfp -dd -log -ma -en";
    char buffer[128];
    std::string args = "";
    std::string disk_input = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run command" << std::endl;
        return 1;
    }
    while (std::fgets(buffer, sizeof(buffer), pipe) != NULL) {
        args += buffer;
        disk_input += buffer;
    }
    pclose(pipe);

    std::stringstream ss(args);
    std::vector<std::string> disk_names;
    std::regex disk_regex(R"(/dev/[a-zA-Z0-9]+)");

    std::smatch match;
    while (std::regex_search(disk_input, match, disk_regex)) {
        disk_names.push_back(match.str());
        disk_input = match.suffix().str();
    }
    
    std::vector<std::string> arg_list;
    while(std::getline(ss, args, ' ')) {
        arg_list.push_back(args);
    }

    TestingFramework tf(arg_list[0], disk_names, arg_list[2], 
    std::stoull(arg_list[3]), std::stoull(arg_list[4]), std::stoull(arg_list[5]), 
    (encoding_t)std::stoull(arg_list[6]), 0);
    tf.printParameters();
    tf.run();
    return 0;
}

