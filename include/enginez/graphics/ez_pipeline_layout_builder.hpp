#pragma once

#include "enginez/graphics/ez_types.hpp"
#include "enginez/graphics/ez_vulkan_backend.hpp"
#include "enginez/utils/ez_inplace_vector.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdint>

namespace enginez::graphics {
    class ezPipelineLayoutBuilder {
      private:
        ezVulkanBackend& backend;
        uint32_t pushConstantRangesOffset = 0;
        utils::inplace_vector<DescriptorSetLayout, 5> descriptorSetLayouts;
        utils::inplace_vector<PushConstantRange, 5> pushConstantRanges;

      public:
        ezPipelineLayoutBuilder(ezVulkanBackend& backend) : backend(backend) {
        }

        ezPipelineLayoutBuilder& reset();

        ezPipelineLayoutBuilder& addSet(DescriptorSetLayout& set);
        ezPipelineLayoutBuilder& addConstant(uint32_t size, VkShaderStageFlags stage);

        ezPipelineLayoutBuilder& build(PipelineLayout& layout);
    };
} // namespace enginez::graphics