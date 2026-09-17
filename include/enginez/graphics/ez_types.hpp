#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "enginez/utils/ez_inplace_vector.hpp"
#include "vma/vk_mem_alloc.h"

namespace enginez::graphics {
//    +----------------------------------------------------+
//    |                        Queue                       |
//    +----------------------------------------------------+
    enum QueueType {
        GRAPHICS = VK_QUEUE_GRAPHICS_BIT,
        COMPUTE = VK_QUEUE_COMPUTE_BIT,
        TRANSFER = VK_QUEUE_TRANSFER_BIT,
        PRESENT,
    };
    inline const char* string_QueueType(QueueType type) {
        switch (type) {
        case GRAPHICS:
            return "GRAPHICS";
        case COMPUTE:
            return "COMPUTE";
        case TRANSFER:
            return "TRANSFER";
        case PRESENT:
            return "PRESENT";
        default:
            return "UNKNOWN";
        }
    }

    struct Queue {
        VkQueue handle;
        uint32_t index;
        uint32_t family;
        QueueType type;
    };

//    +----------------------------------------------------+
//    |                       Memory                       |
//    +----------------------------------------------------+
    enum BufferType {
        INDEX = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VERTEX = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        R_BUFFER = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        RW_BUFFER = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    };
    struct Buffer {
        VkBuffer handle;
        VkDeviceSize size;

        Buffer(VkBuffer handle, VkDeviceSize size) : handle(handle), size(size) {
        }
    };
    struct MemoryBlock {
        MemoryBlock(VkDeviceMemory handle, VkMemoryPropertyFlags properties, uint32_t typeIndex)
            : handle(handle), properties(properties), typeIndex(typeIndex) {
        }
        VkDeviceMemory handle;
        VkMemoryPropertyFlags properties;
        uint32_t typeIndex;
    };

//    +----------------------------------------------------+
//    |                       Images                       |
//    +----------------------------------------------------+

    struct Image {
        VkImage handle;
        VkImageView view;
        VmaAllocation allocation;
        VkExtent3D extent;
    };

//    +----------------------------------------------------+
//    |                       Device                       |
//    +----------------------------------------------------+
    struct PhysicalDevice {
        VkPhysicalDevice handle = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    };
    struct Device {
        VkDevice handle = VK_NULL_HANDLE;
        PhysicalDevice phyisicalDevice;
    };

//    +----------------------------------------------------+
//    |                      Commands                      |
//    +----------------------------------------------------+
    struct CommandPool {
        VkCommandPool handle;
    };

    struct CommandBuffer {
        VkCommandBuffer handle;
    };
    
//    +----------------------------------------------------+
//    |                      Pipelines                     |
//    +----------------------------------------------------+
    struct Shader {
        VkShaderModule handle;
    };

    typedef VkPushConstantRange PushConstantRange;

    struct DescriptorSetLayoutBinding {
        uint32_t count;
        VkDescriptorType type;
        VkShaderStageFlags stage;
    };
    struct DescriptorSetLayout {
        VkDescriptorSetLayout handle;
        utils::inplace_vector<DescriptorSetLayoutBinding, 20> bindings;
    };

    typedef VkDescriptorSet DescriptorSet;
    struct DescriptorPool {
        VkDescriptorPool handle;
    };
    struct PoolSizeRatio{
		VkDescriptorType type;
		float ratio;
    };
    struct PipelineLayout {
        VkPipelineLayout handle;
        utils::inplace_vector<DescriptorSetLayout, 5> descriptorSetLayouts;
    };
    
    struct PipeLine {
        VkPipeline handle;
        PipelineLayout layout;
    };

//    +----------------------------------------------------+
//    |                   Synchronization                  |
//    +----------------------------------------------------+
    typedef VkFence Fence;
    typedef VkSemaphore Semaphore;


    
} // namespace enginez::graphics