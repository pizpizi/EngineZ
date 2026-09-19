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
    glm::float32 density     = 0.01;
    glm::vec2    brushPos    = {0, 0};
    glm::vec2    brushDelta = {0, 0};
    glm::int32   redBlackIdx = 0;
    glm::float32 brushSize   = 1;
    glm::ivec2   simBounds   = {1920, 1200};
    glm::uint32  brushDown   = false;
    glm::uint32  brushType   = 0;
    glm::vec3    brushColor = {1, 1, 1};
    glm::uint32  visType     = 3;
    glm::float32 visScale    = 10;
    glm::float32 overRelaxation = 1.97;
    glm::float32 smokeDiffuse = 10;
    glm::uint32  openEdges = true;
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
    bool shouldClear = false;

    Queue    computeQueue;
    Controls controls;
    int iterations = 150;
    bool alternateRedBlack = false;

    Image pressureMap;
    Image velocityXMap;
    Image velocityXOldMap; 
    Image velocityYMap;
    Image velocityYOldMap; 
    Image smokeMap;
    Image smokeOldMap;
    Image divergenceMap;

    PipeLine brushPipeline;
    PipeLine advectPipeline;
    PipeLine visualizePipeline;
    PipeLine projectPipeline;
    PipeLine velocityUpdatePipeline;
    PipeLine preProcessPipeline;
    PipeLine diffusePipeline;

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

    void blitImages(VkCommandBuffer cmd);
    void clearImages(VkCommandBuffer cmd);

    Image createImage();

    void onMouseMoved(double xpos, double ypos) override;
    void onMouseDown(int button, int action, int mods) override;

  public:
    FluidSimWindow(ezWindowCreateInfo createInfo, Queue& computeQueue) : ezWindow(createInfo), computeQueue(computeQueue) {};
};