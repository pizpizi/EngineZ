#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace enginez::graphics {
    struct VulkanMemoryBlock {
        VkDeviceMemory handle;
        VkMemoryPropertyFlags properties;
        uint32_t typeIndex;
    };
} // namespace enginez::graphics