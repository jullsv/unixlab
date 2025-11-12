#ifndef SHA1_H
#define SHA1_H

#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h> 

std::string calculate_sha1(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "";
    }

    SHA_CTX sha1_context;
    SHA1_Init(&sha1_context);

    char buffer[1024];
    while (file.read(buffer, sizeof(buffer))) {
        SHA1_Update(&sha1_context, buffer, file.gcount());
    }
    if (file.gcount() > 0) {
        SHA1_Update(&sha1_context, buffer, file.gcount());
    }

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1_Final(hash, &sha1_context);

    std::stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return ss.str();
}

#endif // SHA1_H