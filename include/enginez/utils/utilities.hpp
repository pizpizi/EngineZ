#pragma once

#include "GLFW/glfw3.h"
#include <format>
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

inline std::vector<const char*> getGlfwRequiredExtensions(){
    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);

    std::vector<const char*> out;
    
    for(int i = 0 ; i < count; i++){
        out.push_back(extensions[i]);
    }
    
    return out;
}

template<typename T>
inline void setDebugName(VkDevice device, T handle, VkObjectType type, const char* name) {
    VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    info.objectType = type;
    info.objectHandle = (uint64_t)handle;
    info.pObjectName = name;
    PFN_vkSetDebugUtilsObjectNameEXT fn = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
    fn(device, &info);
}

template<typename T, typename... Args>
inline void setDebugName(VkDevice device, T handle, VkObjectType type,
                             std::format_string<Args...> fmt, Args&&... args) {
    std::string name = std::format(fmt, std::forward<Args>(args)...);
    setDebugName(device, handle, type, name.c_str());
}