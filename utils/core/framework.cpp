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

int test_id;

class TestingFramework {
private:
    std:: string traceFilePath;
    std:: vector<std:: string> devices;
    std:: string outputDirPath;
    BIO_Cache* cache;
    uint64_t magic1, magic2, magic3;
    std:: string record_start, record_end, blk2file;
    std:: string rans_path;
public:
    TestingFramework(const std::string& traceFile, const std::vector<std::string>& devices, 
    const std::string& outputDir, const uint64_t& m1, const uint64_t& m2, const uint64_t& m3,
    const std::string& r_start, const std::string& r_end, const std::string& b2f,
    const std::string& rans_path
    )
        : traceFilePath(traceFile), devices(devices), outputDirPath(outputDir), 
        magic1(m1), magic2(m2), magic3(m3),
        record_start(r_start), record_end(r_end), blk2file(b2f),
        rans_path(rans_path)
        {}
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
    }


    void run() {
        // run the core functionality
        std::cout << "Running Core Functionality..." << std::endl;
        try {
            cache = new BIO_Cache(0, 0, magic3, 0);
        } catch (std::exception& e) {
            std::cerr << "Failed to initialize BIO_Cache: " << e.what() << std::endl;
            return;
        }
        cache_BIO();
        
        // launch_ransomware();
        
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
     * @brief we retreive filtered result from blk2file mapping, and then cache related (clean) 
     * blocks in RB tree.
     */
    void cache_BIO() {
        std::string cmd = this->blk2file;
        // the output of command is in format of a string of int separated by space, 
        // we need to get those numbers (block index)
        std::string args = "";
        char buffer[128];
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "Failed to run command" << std::endl;
            return;
        }
        while (std::fgets(buffer, sizeof(buffer), pipe) != NULL) {
            args += buffer;
        }
        pclose(pipe);
        std::stringstream ss(args);
        printf("block numbers obtained : \n");
        while(std::getline(ss, args, ' ')) {
            this->cache->cache(std::stoll(args));
        }
    }
    // we run python3 utils/preprocess.py -run, and wait for it to complete.
    // the output of this process should be re-directed to a file in the output directory
    void launch_ransomware() {
        std::string cmd = "python3 " + rans_path + " -run";
        char buffer[128];
        std::string args = "";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "Failed to run command" << std::endl;
            return;
        }
        while (std::fgets(buffer, sizeof(buffer), pipe) != NULL) {
            args += buffer;
        }
        pclose(pipe);

        // put the output of the python script into a file
        std::ofstream file(outputDirPath + "/ransomware_output.txt");
        file << args;
        file.close();
        std::cout << "Ransomware launched." << std::endl;
        std::cout << "Output file: " << outputDirPath + "/ransomware_output.txt" << std::endl;
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
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        // check if -trace is specified
        if(arg.find("-id=") != std::string::npos) {
            std::string _id = arg.substr(arg.find("=") + 1);
            if (_id == "") {
                std::cerr << "Usage: ./core -id=<id>" << std::endl;
                return -1;
            }
            test_id = std::stoi(_id);
        } 
    }
    return 0;
}


int main(int argc, char** argv) {
    handle_args(argc, argv);
    std::string cmd = "python3 config.py -tfp -dd -log -sys -rans";
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

    TestingFramework tf(arg_list[0], disk_names, arg_list[2], // trace path & device & log dir
    std::stoull(arg_list[3]), std::stoull(arg_list[4]), std::stoull(arg_list[5]),  // 3 magic numbers
    arg_list[6], arg_list[7], arg_list[8],    // blk2file related binaries' path
    arg_list[9]
    );
    tf.printParameters();
    tf.run();
    return 0;
}

