#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

struct QueueFamilyIndices{
    std::vector<uint32_t> graphicQueues = std::vector<uint32_t>();
    std::vector<uint32_t> sparseBindingQueues = std::vector<uint32_t>();
    std::vector<uint32_t> computeQueues = std::vector<uint32_t>();
    std::vector<uint32_t> transferQueues = std::vector<uint32_t>();

    QueueFamilyIndices(VkPhysicalDevice device);
};