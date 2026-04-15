#pragma once

#include "vulkan/vulkan_core.h"

namespace enginez::graphics {
    struct CommandPool{
        CommandPool(VkCommandPool handle) : handle(handle) {
        }
        VkCommandPool handle;
    };

    struct CommandBuffer {
        CommandBuffer(VkCommandBuffer handle) : handle(handle) {
        }
        VkCommandBuffer handle;
    };

}