#pragma once

#include "GLFW/glfw3.h"
#include <fstream>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

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

std::vector<const char*> getGlfwRequiredExtensions(){
    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);

    std::vector<const char*> out;
    
    for(int i = 0 ; i < count; i++){
        out.push_back(extensions[i]);
    }
    
    return out;
}
