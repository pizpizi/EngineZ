#pragma once
#include <vulkan/vulkan_core.h>

namespace enginez::graphics {
    struct Buffer {
        VkBuffer handle;
        VkDeviceSize size;

        Buffer(VkBuffer handle, VkDeviceSize size): handle(handle), size(size){}
    };
    struct MemoryBlock {
        MemoryBlock(VkDeviceMemory handle, VkMemoryPropertyFlags properties, uint32_t typeIndex)
            : handle(handle), properties(properties), typeIndex(typeIndex) {
        }
        VkDeviceMemory handle;
        VkMemoryPropertyFlags properties;
        uint32_t typeIndex;
    };
} // namespace enginez::graphics