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

std::string config_path = "";
std::string trace_info_name = "";
std::string traceFilePath = "";
std::string outputLogPath = "";
std::string logDir = "";
int test_id;


static std::string pathJoin(const std::string& path1, const std::string& path2) {
    if (path1.empty())
        return path2;
    if (path2.empty())
        return path1;

    char lastChar = path1.back();
    if (lastChar != '/' && lastChar != '\\')
        return path1 + '/' + path2;
    else
        return path1 + path2;
}

static std::string readPath(const std::string& path) {
    std::ifstream file(path); 
    if (!file) {
        std::cerr << "Failed to open trace path file." << std::endl;
        return "";
    }
    std::string line;
    std::string traceFilePath = "";
    while(std::getline(file, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
        file.close();
        return line;
    }
    file.close();
    return "";
}

// We pass into some certain flags to config_path and then we get 
// unified configurations (to python scripts)
static void get_path(std::string flags, std::string& path) {
    std::string cmd = "python3 " + config_path + " " + flags;
    char buffer[128];
    std::string args = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run command" << std::endl;
        return ;
    }
    while (std::fgets(buffer, sizeof(buffer), pipe) != NULL) {
        args += buffer;
    }
    pclose(pipe);

    std::stringstream ss(args);
    
    std::vector<std::string> arg_list;
    while(std::getline(ss, args, ' ')) {
        // trim line change and extra space between the args 
        args.erase(std::remove(args.begin(), args.end(), '\n'), args.end());
        arg_list.push_back(args);
    }
    path = arg_list[0];
}

// return -1 on failure, 0 on success
int dumpLog(const std::string& logPath, std::string dumpInfo) {
    std::ofstream file(logPath, std::ios::trunc);
    std::cerr << "Dumping log to " << logPath << std::endl;
    if(!file) {
        std::cerr << "Failed to open log file." << std::endl;
        return -1;
    }
    file << dumpInfo;
    file.close();
    return 0;
}


class TestingFramework {
private:
    std:: string trace_path_file;
    std:: vector<std:: string> devices;
    std:: string outputDirPath;
    BIO_Cache* cache;
    uint64_t magic1, magic2, magic3;
    std:: string record_start, record_end, blk2file;
    std:: string rans_path;
public:
    TestingFramework(const std::string trace_path_file, const std::vector<std::string>& devices, 
    const std::string& outputDir, const uint64_t& m1, const uint64_t& m2, const uint64_t& m3,
    const std::string& r_start, const std::string& r_end, const std::string& b2f,
    const std::string& rans_path
    )
        : trace_path_file(trace_path_file),
        devices(devices), outputDirPath(outputDir), 
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
        std::string outputString = "";
        outputString = "original=" + std::to_string(this->cache->get_unencrypted()) + "\n";
        this->cache->turn_on_rans();
        launch_ransomware();
        this->replay();
        this->cache->turn_off_rans();
        outputString += "final=" + std::to_string(this->cache->get_unencrypted()) + "\n";
        dumpLog(outputLogPath, outputString);
        std::cout<<outputString<<std::endl;
        this->cache->dump_clr_blks(outputLogPath);
        // cache_BIO();
        // this->cache->flush();
        // this->cache->report();
        // this->cache->snapshot();
        // this->cache->snapshot_report(this->tarSysDumpPath);
        // this->cache->debug.dump_snapshot();
        // this->cache->debug.dump_dbg_file();
    }
private:

    void replay() {
        uint64_t secNum, secCount;
        uint64_t i;
        traceFilePath = readPath(this->trace_path_file);
        std::ifstream file;
        std::string line;
        file.open(traceFilePath);
        if (!file) {
            std::cerr << "Failed to open trace file." << std::endl;
            return;
        }
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::vector<std::string> tokens;
            std::string token;
            while (iss >> token) {
                tokens.push_back(token);
            }
            tokens.erase(std::remove(tokens.begin(), tokens.end(), ""), tokens.end());
            // if tokens is not with 5 elements exactly (i.e. not a valid line), skip
            if(tokens.size() != 5){
                continue;
            }
            // if token[3] does not include W, skip
            if(tokens[3].find('W') == std::string::npos){
                continue;
            }
            secNum = std::stoull(tokens[1]);
            secCount = std::stoull(tokens[2]) / 512;
            if(secCount == 0) continue;
            for(i = 0; i < secCount; i++) this->cache->cache(secNum + i);
        }

        file.close();
    }

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
        std::ofstream file(pathJoin(outputDirPath,trace_info_name));
        file << args;
        file.close();
        std::cout << "Ransomware launched." << std::endl;
        std::cout << "Output file: " << pathJoin(outputDirPath,trace_info_name) << std::endl;
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
        } else if(arg.find("-path=") != std::string::npos) {
            std::string _path = arg.substr(arg.find("=") + 1);
            if (_path == "") {
                std::cerr << "Usage: ./core -path=<path>" << std::endl;
                return -1;
            }
            config_path = _path;
        }
    }
    return 0;
}


int main(int argc, char** argv) {
    handle_args(argc, argv);
    // python3 config.py -tfp -dd -log -ma -sys -rans
    std::string cmd = "python3 " + config_path + " -tfp -dd -log -ma -sys -rans";
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
        // trim line change and extra space between the args 
        args.erase(std::remove(args.begin(), args.end(), '\n'), args.end());
        arg_list.push_back(args);
    }

    std::string outputDir = "";
    outputDir = pathJoin(arg_list[2], "logs_" + std::to_string(test_id));

    get_path(" -tinfo", trace_info_name);
    get_path(" -abcdir", logDir);
    logDir = readPath(logDir);
    get_path(" -out", outputLogPath);
    outputLogPath = pathJoin(logDir, outputLogPath);

    TestingFramework tf(arg_list[0], disk_names, outputDir , // trace path & device & log dir
    std::stoull(arg_list[3]), std::stoull(arg_list[4]), std::stoull(arg_list[5]),  // 3 magic numbers
    arg_list[6], arg_list[7], arg_list[8],    // blk2file related binaries' path
    arg_list[9]
    );
    tf.printParameters();
    tf.run();
    return 0;
}

