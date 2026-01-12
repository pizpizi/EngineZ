#pragma once
#include <vulkan/vulkan_core.h>
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace VulkanUtilities {
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    
    enum VkExtendedQueueFlagBits{
        GRAPHICS        = VK_QUEUE_GRAPHICS_BIT,
        COMPUTE         = VK_QUEUE_COMPUTE_BIT,
        TRANSFER        = VK_QUEUE_TRANSFER_BIT,
        SPARSE_BINDING  = VK_QUEUE_SPARSE_BINDING_BIT,
        PROTECTED       = VK_QUEUE_PROTECTED_BIT,
        VIDEO_DECODE    = VK_QUEUE_VIDEO_DECODE_BIT_KHR,
        VIDEO_ENCODE    = VK_QUEUE_VIDEO_ENCODE_BIT_KHR,
        OPTICAL_FLOW    = VK_QUEUE_OPTICAL_FLOW_BIT_NV,
        PRESENT         = 0
    };

    struct QueueFamilyIndices{
        QueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR* surface = NULL);

        void print(int indent = 0);
        
        std::vector<uint32_t>& operator[](VkExtendedQueueFlagBits flagBit) {
            return map[flagBit]; 
        }
    private:
        std::map<VkExtendedQueueFlagBits, std::vector<uint32_t>> map = std::map<VkExtendedQueueFlagBits, std::vector<uint32_t>>();
    };

    struct ImageViewPair{
        VkImage image;
        VkImageView view;
    };
}