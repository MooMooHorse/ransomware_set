#include <iostream>
#include <fstream>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

void process_file(const std::string& file_path) {
    std::ifstream input_file(file_path, std::ios::binary);
    if (!input_file) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }
    std::vector<char> data((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
    input_file.close();

    // revert each byte in the data vector
    for (auto& byte : data) {
        byte = ~byte;
    }

    // write the modified data back to the file
    std::ofstream output_file(file_path, std::ios::binary);
    if (!output_file) {
        std::cerr << "Failed to open file for writing: " << file_path << std::endl;
        return;
    }
    output_file.write(data.data(), data.size());
     // flush the output file buffer and sync to disk
    output_file.flush();
    // sync the output file to disk
    output_file.close();
}

void process_directory(const std::string& dir_path) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (dir_path.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            std::string file_name = ent->d_name;
            if(file_name == "." || file_name == "..") continue;
            std::string file_path = dir_path + "/" + file_name;
            struct stat st;
            if (stat(file_path.c_str(), &st) == -1) {
                std::cerr << "Failed to stat file: " << file_path << std::endl;
                continue;
            }
            if (S_ISDIR(st.st_mode)) {
                // process subdirectory recursively
                process_directory(file_path);
            } else if (S_ISREG(st.st_mode)) {
                // process regular file
                process_file(file_path);
            }
        }
        closedir (dir);
    } else {
        /* could not open directory */
        perror ("");
        return;
    }
}

int main() {
    std::string tar_path = "rans_test/tar_sys";
    process_directory(tar_path);
    sync();
    return 0;
}