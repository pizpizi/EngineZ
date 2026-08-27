#pragma once

#include "enginez/graphics/ez_types.hpp"
#include "enginez/graphics/ez_window.hpp"
#include "glm/glm.hpp"
#include "vulkan/vulkan_core.h"

using namespace enginez;
using namespace enginez::graphics;

struct Controls {
    glm::vec2 brushPos;
    float brushSize = 1;
};

struct FrameData {
  CommandBuffer graphicsBuffer;
};

class FluidSimWindow : public ezWindow {
  private:
    inline static VkExtent3D SIM_BOUNDS {800, 800, 1};
    Queue computeQueue;
    Controls controls;

    Image pressureMap;

    PipeLine pipeline;
    DescriptorPool descriptorPool;
    DescriptorSet mainDescriptorSet;
    DescriptorSetLayout mainDescriptorSetLayout;

    CommandPool computeCommandBufferPool;
    CommandPool graphicsCommandBufferPool;
    CommandBuffer computeCommandBuffer;
    CommandBuffer graphicsCommandBuffer;

    Fence computeFence;
    Semaphore computeSemaphore;

    void onOpen() override;
    void onUpdate() override;
    void onClose() override;

    void createImages();
    void createPipelines();
    void createDescriptorSets();
    void createDescriptorPool();
    void createCommandPool();
    void createCommandBuffer();
    void createSynchObjects();
    void assignDebugNames();

    void onMouseMoved(double xpos, double ypos) override;

  public:
    FluidSimWindow(ezWindowCreateInfo createInfo, Queue& computeQueue) : ezWindow(createInfo), computeQueue(computeQueue) {};
};