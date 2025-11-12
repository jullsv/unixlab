#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sha1.h" 

void find_all_files(const std::string& dir_path, std::vector<std::string>& files) {
    DIR* dir = opendir(dir_path.c_str());
    if (dir == nullptr) {
        std::cerr << "Error: Cannot open directory " << dir_path << std::endl;
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        const std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }

        const std::string full_path = dir_path + "/" + name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == -1) {
            continue;
        }

        if (S_ISREG(st.st_mode)) {
            files.push_back(full_path);
        }
        else if (S_ISDIR(st.st_mode)) {
            find_all_files(full_path, files);
        }
    }

    closedir(dir);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <directory_path>" << std::endl;
        return 1;
    }

    std::string root_dir = argv[1];
    std::vector<std::string> all_files;
    
    find_all_files(root_dir, all_files);

    if (all_files.empty()) {
        return 0;
    }
    
    std::map<std::string, std::vector<std::string>> hash_groups;

    for (const auto& path : all_files) {
        std::string hash = calculate_sha1(path); 
        if (!hash.empty()) {
            hash_groups[hash].push_back(path);
        }
    }

    int links_created = 0;

    for (auto const& [hash, paths] : hash_groups) {
        if (paths.size() > 1) {
            const std::string& original_file = paths[0];
            
            std::cout << "SHA-1: " << hash.substr(0, 10) << "..." << std::endl;
            std::cout << "Original: " << original_file << std::endl;

            for (size_t i = 1; i < paths.size(); ++i) {
                const std::string& duplicate_file = paths[i];
                
                if (unlink(duplicate_file.c_str()) == 0) {
                    if (link(original_file.c_str(), duplicate_file.c_str()) == 0) {
                        links_created++;
                        std::cout << "  -> LINKED: " << duplicate_file << std::endl;
                    } else {
                        std::cerr << "  -> ERROR: Failed to link " << duplicate_file << std::endl;
                    }
                } else {
                    std::cerr << "  -> ERROR: Failed to unlink " << duplicate_file << std::endl;
                }
            }
            std::cout << std::endl;
        }
    }

    std::cout << "Total hard links created: " << links_created << std::endl; 

    return 0;
}