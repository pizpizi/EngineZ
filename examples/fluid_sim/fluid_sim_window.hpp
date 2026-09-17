#pragma once

#include "dxc/dxcapi.h"
#include "enginez/graphics/ez_types.hpp"
#include "enginez/graphics/ez_window.hpp"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "vulkan/vulkan_core.h"

using namespace enginez;
using namespace enginez::graphics;

struct Controls {

    glm::float32 deltaTime   = 1.0 / 165;
    glm::float32 density     = 1;
    glm::vec2    brushPos    = {0, 0};
    glm::int32   redBlackIdx = 0;
    glm::float32 brushSize   = 1;
    glm::ivec2   simBounds   = {1920, 1200};
    glm::uint32  brushDown   = false;
    glm::uint32  brushType   = 1;
    glm::uint32  visType     = 1;
    glm::float32 visScale    = 10;
};

struct FrameData {
    CommandBuffer graphicsBuffer;
};

class FluidSimWindow : public ezWindow {
  private:
    inline static VkExtent3D  SIM_BOUNDS {1920, 1200, 1};
    inline static const char* VISUALIZATION_TYPE[]     = {"Pressure", "Velocity", "Divergence", "Smoke"};
    inline static const int   VISUALIZATION_TYPE_COUNT = 4;
    inline static const char* BRUSH_TYPE[]             = {"Smoke", "Pressure"};
    inline static const int   BRUSH_TYPE_COUNT         = 2;

    bool shouldUpdate = false;
    bool updated = false;
    bool paused = true;

    Queue    computeQueue;
    Controls controls;

    Image pressureMap;
    Image velocityXMap;
    Image velocityXOldMap; 
    Image velocityYMap;
    Image velocityYOldMap; 
    Image smokeMap;
    Image smokeOldMap;

    PipeLine brushPipeline;
    PipeLine advectPipeline;
    PipeLine visualizePipeline;
    PipeLine projectPipeline;
    PipeLine velocityUpdatePipeline;

    DescriptorSetLayout computeDSLayout;
    DescriptorPool      descriptorPool;
    DescriptorSet       computeDS;

    CommandBuffer computeCommandBuffer;
    CommandBuffer graphicsCommandBuffer;
    CommandPool   computeCommandBufferPool;
    CommandPool   graphicsCommandBufferPool;

    Fence     computeFence;
    Semaphore computeSemaphore;

    void onOpen() override;
    void draw(Image& drawImage) override;
    void onClose() override;

    void createImages();
    void createPipelines();
    void createDescriptorSets();
    void createDescriptorPool();
    void createCommandPool();
    void createCommandBuffer();
    void createSynchObjects();
    void assignDebugNames();

    Image createImage();

    void onMouseMoved(double xpos, double ypos) override;
    void onMouseDown(int button, int action, int mods) override;

  public:
    FluidSimWindow(ezWindowCreateInfo createInfo, Queue& computeQueue) : ezWindow(createInfo), computeQueue(computeQueue) {};
};