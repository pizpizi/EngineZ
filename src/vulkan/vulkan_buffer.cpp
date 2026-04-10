#include "vulkan_buffer.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

using namespace enginez::graphics;
void VulkanBuffer::cleanUp() {
    vkFreeMemory(device, handle, nullptr);
}

void VulkanBuffer::download(void* ptr, size_t size, size_t offset) {
    void* mappedMemory;
    vkMapMemory(device, handle, offset, size, 0, &mappedMemory);
    memcpy(ptr, mappedMemory, size);
    vkUnmapMemory(device, handle);
}

void VulkanBuffer::upload(void* ptr, size_t size, size_t offset) {
    void* mappedMemory;
    vkMapMemory(device, handle, offset, size, 0, &mappedMemory);
    memcpy(mappedMemory, ptr, size);
    vkUnmapMemory(device, handle);
}

VulkanBuffer::VulkanBuffer(VkDevice device, uint32_t typeIndex, size_t size) {
    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = size;
    allocateInfo.memoryTypeIndex = typeIndex;

    this->device = device;

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate memory");
    }
}