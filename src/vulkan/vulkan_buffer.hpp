#pragma once
#include <vulkan/vulkan_core.h>

namespace enginez::graphics {
    struct VulkanBuffer {
        VkBuffer handle;
        VkDeviceSize size;
    };
} // namespace enginez::graphics