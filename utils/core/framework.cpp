#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstdio>
#include <string>
#include <algorithm>
#include "bio.h"

class TestingFramework {
private:
    std:: string traceFilePath;
    std:: string diskDevice;
    std:: string outputDirPath;
    BIO_Cache* cache;
    uint64_t magic1, magic2, magic3;
public:
    TestingFramework(const std::string& traceFile, const std::string& device, const std::string& outputDir, 
    const uint64_t& m1, const uint64_t& m2, const uint64_t& m3)
        : traceFilePath(traceFile), diskDevice(device), outputDirPath(outputDir), magic1(m1), magic2(m2), magic3(m3) {}
    void printParameters() const {
        // core functionality initialized
        std::cout << "Core Functionality Initialized..." << std::endl;
        std::cout << "Trace File: " << traceFilePath << std::endl;
        std::cout << "Disk Device: " << diskDevice << std::endl;
        std::cout << "Output Directory: " << outputDirPath << std::endl;
        std::cout << "Magic Numbers: " << magic1 << " " << magic2 << " " << magic3 << std::endl;
    }


    void run() {
        // run the core functionality
        std::cout << "Running Core Functionality..." << std::endl;
        try {
            cache = new BIO_Cache(0, 0, 0, 0, magic1, magic2, magic3, this->diskDevice);
        } catch (std::exception& e) {
            std::cerr << "Failed to initialize BIO_Cache: " << e.what() << std::endl;
            return;
        }
        cache_BIO();
        this->cache->flush();
        this->cache->report();
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

};



int main() {
    std::string cmd = "python3 config.py -tfp -dd -cd -ma";
    char buffer[128];
    std::string args = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run command" << std::endl;
        return 1;
    }
    while (std::fgets(buffer, sizeof(buffer), pipe) != NULL) {
        args += buffer;
    }
    pclose(pipe);

    std::stringstream ss(args);
    
    std::vector<std::string> arg_list;
    while(std::getline(ss, args, ' ')) {
        arg_list.push_back(args);
    }

    TestingFramework tf(arg_list[0], arg_list[1], arg_list[2], std::stoull(arg_list[3]), std::stoull(arg_list[4]), std::stoull(arg_list[5]));
    tf.printParameters();
    tf.run();
}

