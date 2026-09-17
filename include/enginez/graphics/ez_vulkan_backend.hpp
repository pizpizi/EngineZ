#pragma once

#include "enginez/graphics/ez_error.hpp"
#include "ez_types.hpp"
#include "logz/logger.hpp"
#include <cstdint>
#include <expected>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace enginez {
    class ezEngine;
}
namespace enginez::graphics {
    class ezWindow;
    class ezImageBuilder;

    class ezVulkanBackend {
        friend ezWindow;
        friend ezImageBuilder;

      public:
        inline static const VkFormat DESIRED_SWAPCHAIN_COLOR_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
        inline static const VkFormat DESIRED_COLOR_FORMAT           = VK_FORMAT_R16G16B16A16_SFLOAT;
        inline static const VkFormat DESIRED_DEPTH_FORMAT           = VK_FORMAT_D32_SFLOAT;
        inline static const VkColorSpaceKHR DESIRED_COLOR_SPACE     = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        inline static const VkImageSubresourceRange SUBRESOURCE_WHOLE {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        };

        inline static const VkImageSubresourceLayers SUBRESOURCE_LAYERS_WHOLE {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = 0,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        };

        VkInstance instance;
        VmaAllocator allocator;
        Device device;

        void init(std::vector<Queue>& deviceQueues);
        void cleanUp();
        void update();

        ezWindow* addWindow(ezWindow* window);

        // --------- memory blocks ---------- //
        std::optional<MemoryBlock> allocateMemory(size_t size);
        void downloadFromMemory(MemoryBlock& src, void* dst, size_t size, size_t offset);
        void uploadToMemory(MemoryBlock& dst, void* src, size_t size, size_t offset);
        void cleanUpMemoryBlock(MemoryBlock& memoryBlock);
        // ---------------------------------- //

        // ------------ shaders ------------- //
        std::optional<Shader> createShader(const char* filePath);
        void cleanUpShader(Shader& shader);
        // ---------------------------------- //

        // ------------ buffers ------------- //
        std::optional<Buffer> createBuffer(size_t size, BufferType type, MemoryBlock& boundMemoryId, size_t offset);
        void cleanUpBuffer(Buffer& buffer);
        // ---------------------------------- //

        // ----------- descriptor ----------- //
        std::expected<DescriptorPool, err::Code> createDescriptorSetPool(std::span<VkDescriptorPoolSize> poolSize);
        void cleanUpcreateDescriptorSetPool(DescriptorPool layout);

        err::Code allocateDescriptorSets(DescriptorPool pool, uint32_t count, DescriptorSetLayout& pLayouts, DescriptorSet* pDescriptorSets);

        err::Code updateDescriptorSets(uint32_t writesSize, VkWriteDescriptorSet* writes, uint32_t copiesSize, VkCopyDescriptorSet* copies);
        // ---------------------------------- //

        // --------- command buffers -------- //
        std::optional<CommandPool> createCommandPool(Queue& queue);
        std::optional<CommandBuffer> allocateCommandBuffer(CommandPool& pool);
        // ---------------------------------- //

        // ------------ pipelines ----------- //
        std::optional<PipeLine> createGraphicsPipeline(Shader frag, Shader vert, PipelineLayout& layout);
        std::expected<PipeLine, err::Code> createComputePipeline(Shader& computeShader, PipelineLayout& layout);
        // ---------------------------------- //

        // -------------- sync -------------- //
        std::optional<Semaphore> createSemaphore();
        std::optional<Fence> createFence();
        // ---------------------------------- //

        bool submitAndSynchronize(CommandBuffer commandBuffer, Queue& queue);

      private:
        logz::DefaultLogger& logger                = logz::createDefaultLogger(logz::SINCE_PROGRAM_START, "Vulkan");
        logz::DefaultLogger& validationLayerLogger = logz::createDefaultLogger(logz::SINCE_PROGRAM_START, "Validation Layer");

        std::vector<const char*> requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        
        std::vector<MemoryBlock> memoryBlocks;
        std::vector<Shader> shaders;
        std::vector<Buffer> buffers;
        std::vector<PipeLine> pipelines;
        std::vector<ezWindow*> windows;

        VkDebugUtilsMessengerEXT debugMessenger;

        void setupLogger();
        void setupInstance();
        void setupDebugMessenger();
        void setupAllocator();
        PhysicalDevice choosePhysicalDevice(std::vector<Queue> deviceQueues);
        Device setupLogicalDevice(std::vector<Queue>& deviceQueues, PhysicalDevice phyisicalDevice);
        void setupCommandBuffer();

        bool assignQueues(PhysicalDevice& physicalDevice, std::vector<Queue>& queues);

        int32_t getSuitableMemoryType(Device& logicalDevice, VkMemoryPropertyFlags requiredFlags);

        VkBool32 debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
        );

        static VKAPI_ATTR VkBool32 VKAPI_CALL baseDebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData
        );
    };

}; // namespace enginez::graphics