#pragma once

#include "vulkan/vulkan_core.h"

namespace enginez::graphics {
    struct PipeLine {
        PipeLine(VkPipeline handle) : handle(handle) {
        }
        VkPipeline handle;
    };
    struct PipelineLayout {
        PipelineLayout(VkPipelineLayout handle) : handle(handle) {
        }
        VkPipelineLayout handle;
    };
    struct Shader {
        Shader(VkShaderModule handle) : handle(handle) {
        }
        VkShaderModule handle;
    };

    typedef VkDescriptorSetLayout DescriptorSetLayout;
    typedef VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding;
    typedef VkDescriptorSet DescriptorSet;
    struct DescriptorPool {
        DescriptorPool(VkDescriptorPool handle) : handle(handle) {
        }
        VkDescriptorPool handle;
    };
    typedef VkPushConstantRange PushConstantRange ;
} // namespace enginez::graphics