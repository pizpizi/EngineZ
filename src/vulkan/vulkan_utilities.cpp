#include "enginez/graphics/vulkan_utilities.hpp"
#include "GLFW/glfw3.h"
#include <cstdint>

std::vector<const char*> getGlfwRequiredExtensions(){
    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);

    std::vector<const char*> out;
    
    for(int i = 0 ; i < count; i++){
        out.push_back(extensions[i]);
    }
    
    return out;
}