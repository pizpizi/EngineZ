#include "enginez/graphics/ez_image_builder.hpp"
#include "vulkan/vulkan_core.h"

using namespace enginez::graphics;

ezImageBuilder& ezImageBuilder::reset() {
    format = VK_FORMAT_UNDEFINED;
    usage  = 0;
    extent = {0, 0, 0};

    familyIndecies.clear();
    layout = VK_IMAGE_LAYOUT_GENERAL;

    return *this;
}
ezImageBuilder& ezImageBuilder::setFormat(VkFormat format) {
    this->format = format;

    return *this;
}
ezImageBuilder& ezImageBuilder::setExtent(VkExtent3D extent) {
    this->extent = extent;

    return *this;
}

ezImageBuilder& ezImageBuilder::setLayout(VkImageLayout layout){
    this->layout = layout;

    return *this;
}

ezImageBuilder& ezImageBuilder::setExtent(VkExtent2D extent) {
    this->extent.width  = extent.width;
    this->extent.height = extent.height;
    this->extent.depth  = 1;

    return *this;
}
ezImageBuilder& ezImageBuilder::setUsage(VkImageUsageFlags usage) {
    this->usage = usage;

    return *this;
}

ezImageBuilder& ezImageBuilder::addFamily(uint32_t family) {
    for (auto& f : familyIndecies) {
        if (f == family) return *this;
    }

    familyIndecies.push_back(family);
    if (familyIndecies.size() > 1) sharingMode = VK_SHARING_MODE_CONCURRENT;

    return *this;
}
ezImageBuilder& ezImageBuilder::resetFamilies() {
    familyIndecies.clear();
    sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    return *this;
}

ezImageBuilder& ezImageBuilder::build(Image& image) {
    VkImageCreateInfo imageCI {
        .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .imageType             = VK_IMAGE_TYPE_2D,
        .format                = format,
        .extent                = extent,
        .mipLevels             = 1,
        .arrayLayers           = 1,
        .samples               = VK_SAMPLE_COUNT_1_BIT,
        .tiling                = VK_IMAGE_TILING_OPTIMAL,
        .usage                 = usage,
        .sharingMode           = sharingMode,
        .queueFamilyIndexCount = static_cast<uint32_t>(familyIndecies.size()),
        .pQueueFamilyIndices   = familyIndecies.data(),
        .initialLayout         = layout,
    };

    VmaAllocationCreateInfo allocationCI {
        .usage         = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    vmaCreateImage(backend.allocator, &imageCI, &allocationCI, &image.handle, &image.allocation, nullptr);

    VkImageViewCreateInfo viewCI {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .image            = image.handle,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = format,
        .components       = {},
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };

    vkCreateImageView(backend.device.handle, &viewCI, nullptr, &image.view);

    image.extent = extent;

    return *this;
}