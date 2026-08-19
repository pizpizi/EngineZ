#pragma once
#include "enginez/graphics/command_buffers.hpp"
#include "logz/logger.hpp"
#include "pipelines.hpp"
#include "resources.hpp"
#include "vulkan_window.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace enginez {
    class Engine;
}
namespace enginez::graphics {

    enum BufferType {
        INDEX = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VERTEX = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        R_BUFFER = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        RW_BUFFER = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    };
    enum QueueType {
        GRAPHICS = VK_QUEUE_GRAPHICS_BIT,
        COMPUTE = VK_QUEUE_COMPUTE_BIT,
        TRANSFER = VK_QUEUE_TRANSFER_BIT,
        PRESENT,
    };
    struct Queue {
        VkQueue handle;
        uint32_t index;
        uint32_t family;
        QueueType type;
    };
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

    class VulkanBackend {
      public:
        /**
         * @brief Initializes the Vulkan instance and device.
         * * @param deviceQueues A vector to be populated with requested queue handles.
         */
        void init(std::vector<Queue>& deviceQueues);
        void cleanUp();

        VulkanWindow* createWindow(std::string title, int width, int height);

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
        std::optional<DescriptorSetLayout> createDescriptorSetLayout(VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount);
        void cleanUpDescriptorSetLayout(DescriptorSetLayout layout);

        std::optional<DescriptorPool> createDescriptorSetPool(std::map<VkDescriptorType, uint32_t> resourceCount, uint32_t maxSets);
        void cleanUpcreateDescriptorSetPool(DescriptorPool layout);

        bool allocateDescriptorSets(DescriptorPool pool, uint32_t count, DescriptorSetLayout* pLayouts, DescriptorSet* pDescriptorSets);

        void updateDescriptorSets(std::vector<VkWriteDescriptorSet> writes, std::vector<VkCopyDescriptorSet> copies);
        // ---------------------------------- //

        // --------- command buffers -------- //
        std::optional<CommandPool> createCommandPool(Queue& queue);
        std::optional<CommandBuffer> allocateCommandBuffer(CommandPool& pool);
        // ---------------------------------- //

        std::optional<PipeLine> createComputePipeline(Shader& computeShader, PipelineLayout& layout);
        std::optional<PipelineLayout> createPipelineLayout(uint32_t descriptorSetCount, DescriptorSetLayout* pDescriptorSetLayouts,
                                                           uint32_t pushConstantRangesCount, VkPushConstantRange* pPushConstantRanges);

        bool submitAndSynchronize(CommandBuffer commandBuffer, Queue& queue);

      private:
        logz::DefaultLogger& logger = logz::createDefaultLogger(logz::SINCE_PROGRAM_START, "Vulkan");
        logz::DefaultLogger& validationLayerLogger = logz::createDefaultLogger(logz::SINCE_PROGRAM_START, "Validation Layer");

        std::vector<const char*> requiredDeviceExtensions = {};
        std::map<VkQueueFlagBits, uint8_t> requiredQueues = {{VK_QUEUE_COMPUTE_BIT, 1}};

        std::vector<MemoryBlock> memoryBlocks;
        std::vector<Shader> shaders;
        std::vector<Buffer> buffers;
        std::vector<VulkanWindow*> windows;

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        Device logicalDevice;
        VkPipeline computePipeline;

        void setupLogger();
        void setupInstance();
        void setupDebugMessenger();
        PhysicalDevice choosePhysicalDevice(std::vector<Queue> deviceQueues);
        Device setupLogicalDevice(std::vector<Queue>& deviceQueues, PhysicalDevice phyisicalDevice);
        void setupCommandBuffer();

        bool assignQueues(std::vector<VkQueueFamilyProperties>& queueFamilyProperties, std::vector<Queue>& queues);

        void update();

        int32_t getSuitableMemoryType(Device& logicalDevice, VkMemoryPropertyFlags requiredFlags);

        VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                               const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);

        static VKAPI_ATTR VkBool32 VKAPI_CALL baseDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    };

}; // namespace enginez::graphics