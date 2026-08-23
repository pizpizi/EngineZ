#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
#include "enginez/ez_engine.hpp"
#include "enginez/graphics/ez_error.hpp"
#include "enginez/graphics/ez_types.hpp"
#include "enginez/graphics/ez_vulkan_backend.hpp"
#include "enginez/graphics/ez_window.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "logz/logger.hpp"
#include <charconv>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <sys/types.h>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace enginez;
using namespace enginez::graphics;
using namespace std;

class TestWindow : public ezWindow {
  public:
    TestWindow(ezWindowCreateInfo createInfo) : ezWindow(createInfo) {};

    DescriptorSet ds;
    PipeLine pl;
    PipelineLayout plLayout;

    void onOpen() override {
        VkDescriptorSetLayoutBinding binding {
            .binding         = 0,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT,
        };

        auto dsLayout = backend.createDescriptorSetLayout(&binding, 1).value();
        plLayout      = backend.createPipelineLayout(1, &dsLayout, 0, nullptr).value();

        std::vector<VkDescriptorPoolSize> sizes = {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

        auto dsPool = backend.createDescriptorSetPool(sizes).value();

        backend.allocateDescriptorSets(dsPool, 1, &dsLayout, &ds);

        VkDescriptorImageInfo imageInfo {
            .sampler     = nullptr,
            .imageView   = image.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };

        VkWriteDescriptorSet write {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = ds,
            .dstBinding      = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = &imageInfo,
        };

        backend.updateDescriptorSets(1, &write, 0, nullptr);

        auto shader = backend.createShader("shaders/gradient.comp.spv").value();

        pl = backend.createComputePipeline(shader, plLayout).value();
    }
    void onUpdate() override {
        vkCmdBindPipeline(currentFrameData->commandBuffer.handle, VK_PIPELINE_BIND_POINT_COMPUTE, pl.handle);

        vkCmdBindDescriptorSets(currentFrameData->commandBuffer.handle, VK_PIPELINE_BIND_POINT_COMPUTE, plLayout.handle, 0, 1, &ds, 0, nullptr);

        vkCmdDispatch(currentFrameData->commandBuffer.handle, std::ceil(image.extent.width / 16.f), std::ceil(image.extent.height / 16.f), 1);

        drawGUI();
    }

    void drawGUI(){
        ImGui::Begin("Another Window");
        ImGui::Text("Hello from another window!");
        ImGui::End();
    }

    void onClose() override {
        engine.terminate();
    }
};
int main() {
    vector<Queue> queues {{.type = GRAPHICS}, {.type = PRESENT}};
    EngineCreateInfo createInfo = {.deviceQueues = queues};

    try {
        ezEngine engine(createInfo);

        ezWindowCreateInfo windowCI {
            .graphicsQueue = queues[0],
            .presentQueue  = queues[1],
            .engine        = engine,
            .title         = "K",
            .width         = 800,
            .height        = 600,
        };
        auto window = new TestWindow(windowCI);

        engine.graphicsBackend.addWindow(window);
        engine.loop();
    } catch (const err::ezError& e) {
        std::cerr << "Error adding window: " << e.what() << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Non-engine error adding window: " << e.what() << '\n';
    } catch (...) {
        std::cerr << "Unknown exception adding window\n";
    }
}