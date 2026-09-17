
#include "enginez/graphics/ez_descriptor_set_layout_builder.hpp"
#include <cstdint>

using namespace enginez::graphics;

ezDescriptorSetLayoutBuilder& ezDescriptorSetLayoutBuilder::reset() {
    bindings.clear();

    return *this;
}
ezDescriptorSetLayoutBuilder& ezDescriptorSetLayoutBuilder::addBinding(VkDescriptorType type, VkShaderStageFlags stage) {
    bindings.push_back({
        .count = 1,
        .type  = type,
        .stage = stage,
    });

    return *this;
}
ezDescriptorSetLayoutBuilder& ezDescriptorSetLayoutBuilder::build(DescriptorSetLayout& descriptorSetLayout) {
    VkDescriptorSetLayoutBinding bindingInfos[20];
    for (uint32_t i = 0; i < bindings.size(); i++) {
        auto& b         = bindings[i];
        bindingInfos[i] = VkDescriptorSetLayoutBinding {
            .binding            = i,
            .descriptorType     = b.type,
            .descriptorCount    = b.count,
            .stageFlags         = b.stage,
            .pImmutableSamplers = nullptr,
        };
    }

    VkDescriptorSetLayoutCreateInfo createInfo {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings    = bindingInfos,
    };

    vkCreateDescriptorSetLayout(backend.device.handle, &createInfo, nullptr, &descriptorSetLayout.handle);

    descriptorSetLayout.bindings = bindings;

    return *this;
}