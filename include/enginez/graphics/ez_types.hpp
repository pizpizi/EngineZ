#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>
#include "GLFW/glfw3.h"
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
        VkFormat format;
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
    struct PipeLine {
        VkPipeline handle;
    };
    struct PipelineLayout {
        VkPipelineLayout handle;
    };
    struct Shader {
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
    struct PoolSizeRatio{
		VkDescriptorType type;
		float ratio;
    };
    typedef VkPushConstantRange PushConstantRange;

//    +----------------------------------------------------+
//    |                   Synchronization                  |
//    +----------------------------------------------------+
    typedef VkFence Fence;
    typedef VkSemaphore Semaphore;


    
} // namespace enginez::graphics