#pragma once

#include "enginez/graphics/buffer.hpp"
#include <vulkan/vulkan_core.h>
namespace enginez::graphics {
    class VulkanBackend;
    class VulkanBuffer : public enginez::graphics::Buffer {
      public:
        void cleanUp() override;
        void download(void* ptr, size_t size, size_t offset) override;
        void upload(void* ptr, size_t size, size_t offset) override;

      private:
        friend VulkanBackend;
        VulkanBuffer(VkDevice device, uint32_t typeIndex, size_t size);
        VkDevice device;
        VkDeviceMemory handle;
    };

} // namespace enginez::graphics