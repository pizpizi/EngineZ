#pragma once

#include <fstream>
#include <string>

inline std::string ReadBinaryFile(const char* filepath) {
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return {};
    }
    
    size_t fileSize = (size_t)file.tellg();
    std::string buffer;

    buffer.resize(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}