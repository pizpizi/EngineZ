#pragma once

#include "enginez/graphics/ez_types.hpp"
#include "enginez/graphics/ez_vulkan_backend.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <vector>
namespace enginez::graphics {
    class ezImageBuilder {
      private:
        ezVulkanBackend& backend;

        std::vector<uint32_t> familyIndecies = {};
        VkImageUsageFlags     usage          = 0;
        VkFormat              format         = VK_FORMAT_UNDEFINED;
        VkExtent3D            extent         = {0, 0, 0};
        VkImageLayout         layout         = VK_IMAGE_LAYOUT_GENERAL;
        VkSharingMode         sharingMode    = VK_SHARING_MODE_EXCLUSIVE;

      public:
        ezImageBuilder(ezVulkanBackend& backend) : backend(backend) {
        }

        ezImageBuilder& reset();
        ezImageBuilder& setFormat(VkFormat format);
        ezImageBuilder& setUsage(VkImageUsageFlags usage);
        ezImageBuilder& setLayout(VkImageLayout layout);
        ezImageBuilder& setExtent(VkExtent3D extent);
        ezImageBuilder& setExtent(VkExtent2D extent);
        ezImageBuilder& addFamily(uint32_t family);
        ezImageBuilder& resetFamilies();

        ezImageBuilder& build(Image& image);
    };
} // namespace enginez::graphics