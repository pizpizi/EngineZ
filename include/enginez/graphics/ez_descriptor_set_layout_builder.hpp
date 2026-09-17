#pragma once

#include "enginez/graphics/ez_types.hpp"
#include "enginez/graphics/ez_vulkan_backend.hpp"
namespace enginez::graphics {
    class ezDescriptorSetLayoutBuilder {
      private:
        ezVulkanBackend& backend;
        utils::inplace_vector<DescriptorSetLayoutBinding, 20> bindings;

      public:
        ezDescriptorSetLayoutBuilder(ezVulkanBackend& backend) : backend(backend) {
        }

        ezDescriptorSetLayoutBuilder& reset();
        ezDescriptorSetLayoutBuilder& addBinding(VkDescriptorType type, VkShaderStageFlags stage);
        ezDescriptorSetLayoutBuilder& build(DescriptorSetLayout& descriptorSetLayout);
    };
} // namespace enginez::graphics