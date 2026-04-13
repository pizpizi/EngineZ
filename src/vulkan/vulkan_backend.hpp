#pragma once
#include "enginez/graphics/graphics_backend.hpp"
#include "logz/logger.hpp"
#include "non_owning_object_registry.hpp"
#include "vulkan/vulkan_buffer.hpp"
#include "vulkan/vulkan_memory_block.hpp"
#include "vulkan/vulkan_shader.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace enginez::graphics {
    struct Queue {
        VkQueue handle;
        uint32_t index;
        uint32_t family;
    };
    struct LogicalDevice {
        VkDevice device;
        VkPhysicalDevice physicalDevice;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memoryProperties;
    };
    class VulkanBackend final : public GraphicsBackend {
      public:
        void init() override;
        void cleanUp() override;

        EnginezWindow* createWindow(std::string title, int width, int height) override;

        // --------- memory blocks ---------- //
        MemoryBlockId allocateMemory(size_t size) override;
        void downloadFromMemory(MemoryBlockId srcId, void* dst, size_t size, size_t offset) override;
        void uploadToMemory(MemoryBlockId dstId, void* src, size_t size, size_t offset) override;
        void cleanUpMemoryBlock(MemoryBlockId memoryBlock) override;
        // ---------------------------------- //

        // ------------- shaders ------------ //
        ShaderId createShader(const char* filePath) override;
        void cleanUpShader(ShaderId shader) override;
        // ---------------------------------- //

        // ------------- buffers ------------ //
        BufferId createBuffer(size_t size, BufferType type, MemoryBlockId boundMemoryId, size_t offset) override;
        void cleanUpBuffer(BufferId id) override;
        // ---------------------------------- //

        PipelineHandle createComputePipeline(ShaderId computeShader) override;

      private:
        logz::DefaultLogger& logger = logz::createDefaultLogger(logz::SINCE_PROGRAM_START, "Vulkan");
        logz::DefaultLogger& validationLayerLogger = logz::createDefaultLogger(logz::SINCE_PROGRAM_START, "Validation Layer");

        std::vector<const char*> requiredDeviceExtensions = {};
        std::map<VkQueueFlagBits, uint8_t> requiredQueues = {{VK_QUEUE_COMPUTE_BIT, 1}};

        ObjectRegistry<VulkanMemoryBlock, MemoryBlockId> memoryBlocksRegistery;
        ObjectRegistry<VulkanShader, ShaderId> shadersRegistry;
        ObjectRegistry<VulkanBuffer, BufferId> bufferRegistry;

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        LogicalDevice logicalDevice;
        Queue computeQueue;

        void setupLogger();
        void setupInstance();
        void setupDebugMessenger();
        void setupPhysicalDevice();
        void setupLogicalDevice();

        int32_t getSuitableMemoryType(LogicalDevice& logicalDevice, VkMemoryPropertyFlags requiredFlags);

        VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                               const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);

        static VKAPI_ATTR VkBool32 VKAPI_CALL baseDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    };

}; // namespace enginez::graphics