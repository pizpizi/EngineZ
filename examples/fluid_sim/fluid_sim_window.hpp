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
    glm::float32 deltaTime      = 1.0 / 165;
    glm::float32 density        = 0.01;
    glm::vec2    brushPos       = {0, 0};
    glm::vec2    brushDelta     = {0, 0};
    glm::int32   redBlackIdx    = 0;
    glm::float32 brushSize      = 1;
    glm::ivec2   simBounds      = {640, 400};
    glm::uint32  brushDown      = false;
    glm::uint32  brushType      = 0;
    glm::vec3    brushColor     = {1, 1, 1};
    glm::uint32  visType        = 3;
    glm::ivec2   drawBounds     = {0, 0};
    glm::float32 visScale       = 10;
    glm::float32 overRelaxation = 1.8;
    glm::float32 smokeDiffuse   = 10;
    glm::float32 cellSize       = 1;
    glm::uint32  openEdges      = true;
    glm::uint32  cellType;
    glm::vec4    cellData;
};

struct FrameData {
    CommandBuffer graphicsBuffer;
};

class FluidSimWindow : public ezWindow {
  private:
    inline static VkExtent3D SIM_BOUNDS {640, 400, 1};
    inline static const int FRAMES_IN_FLY = 1;
    enum BRUSH_TYPE { BRUSH_SMOKE, BRUSH_PRESSURE, BRUSH_CELL };
    enum CELL_TYPE { CELL_AIR, CELL_SOLID, CELL_SMOKE, CELL_VELOCITY, CELL_PRESSURE };
    enum VISUALIZATION_TYPE {
        VISUALIZE_PRESSURE,
        VISUALIZE_VELOCITY,
        VISUALIZE_DIVERGENCE,
        VISUALIZE_SMOKE,
    };

    inline static utils::inplace_vector<const char*, 10> VISUALIZATION_TYPE_STRING = {"Pressure", "Velocity", "Divergence", "Smoke"};
    inline static utils::inplace_vector<const char*, 10> BRUSH_TYPE_STRING         = {"Smoke", "Velocity", "Cell"};
    inline static utils::inplace_vector<const char*, 10> CELL_TYPE_STRING          = {"Air", "Solid", "Smoke", "Velocity", "Pressure"};

    bool shouldUpdate = false;
    bool updated      = false;
    bool paused       = false;
    bool shouldClear  = false;

    Queue    computeQueue;
    Controls controls;
    int      iterations = 150;
    int currentFrame = 0;

    Image pressureMap;
    Image velocityXMap;
    Image velocityXOldMap;
    Image velocityYMap;
    Image velocityYOldMap;
    Image smokeMap;
    Image smokeOldMap;
    Image divergenceMap;
    Image solidityMap;
    Image cellDataMap;

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

    CommandBuffer computeCommandBuffer[FRAMES_IN_FLY];
    CommandBuffer graphicsCommandBuffer;
    CommandPool   computeCommandBufferPool;
    CommandPool   graphicsCommandBufferPool;

    Fence     computeFence[FRAMES_IN_FLY];
    Semaphore computeSemaphore[FRAMES_IN_FLY];

    void onOpen() override;
    void draw(Image& drawImage, uint64_t deltaTime) override;
    void onClose() override;

    void createImages();
    void createPipelines();
    void createDescriptorSets();
    void createDescriptorPool();
    void createCommandPool();
    void createCommandBuffer();
    void createSynchObjects();
    void setImagesLayouts();
    void assignDebugNames();

    void blitImages(VkCommandBuffer cmd);
    void clearImages(VkCommandBuffer cmd);

    Image createImage();

    void onMouseMoved(double xpos, double ypos) override;
    void onMouseDown(int button, int action, int mods) override;
  public:
    FluidSimWindow(ezWindowCreateInfo createInfo, Queue& computeQueue) : ezWindow(createInfo), computeQueue(computeQueue) {};
};