#include "enginez/graphics/ez_pipeline_layout_builder.hpp"
#include "enginez/graphics/ez_types.hpp"
#include "enginez/utils/ez_inplace_vector.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdint>

using namespace enginez::graphics;

ezPipelineLayoutBuilder& ezPipelineLayoutBuilder::reset() {
    descriptorSetLayouts.clear();
    pushConstantRanges.clear();

    return *this;
}

ezPipelineLayoutBuilder& ezPipelineLayoutBuilder::addSet(DescriptorSetLayout& set) {
    descriptorSetLayouts.push_back(set);
    return *this;
}

ezPipelineLayoutBuilder& ezPipelineLayoutBuilder::addConstant(uint32_t size, VkShaderStageFlags stage) {
    pushConstantRanges.push_back({
        .stageFlags = stage,
        .offset     = pushConstantRangesOffset,
        .size       = size,
    });
    pushConstantRangesOffset += size;

    return *this;
}

ezPipelineLayoutBuilder& ezPipelineLayoutBuilder::build(PipelineLayout& layout) {
    utils::inplace_vector<VkDescriptorSetLayout, 5> tmp_descriptorSetLayout;
    for (auto i = 0; i < descriptorSetLayouts.size(); i++) {
        tmp_descriptorSetLayout.push_back(descriptorSetLayouts[i].handle);
    }

    VkPipelineLayoutCreateInfo layoutCreateInfo {};
    layoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount         = static_cast<uint32_t>(tmp_descriptorSetLayout.size());
    layoutCreateInfo.pSetLayouts            = tmp_descriptorSetLayout.data();
    layoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    layoutCreateInfo.pPushConstantRanges    = pushConstantRanges.data();

    vkCreatePipelineLayout(backend.device.handle, &layoutCreateInfo, nullptr, &layout.handle);
    layout.descriptorSetLayouts = descriptorSetLayouts;

    return *this;
}