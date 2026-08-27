#include "glm/fwd.hpp"
#include <cstdint>
#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
#include "enginez/ez_engine.hpp"
#include "enginez/graphics/ez_error.hpp"
#include "enginez/graphics/ez_types.hpp"
#include "enginez/graphics/ez_vulkan_backend.hpp"
#include "enginez/graphics/ez_window.hpp"
#include "glm/glm.hpp"
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
  private:
    struct PushConstants {
        glm::vec4 data1;
        glm::vec4 data2;
        glm::vec4 data3;
        glm::vec4 data4;
    };

    struct Effect {
        const char* name;
        PipeLine pipeline;

        PushConstants data;
        DescriptorSet descriptorSet;
    };

    vector<Effect> effects;
    uint8_t chosenPipeline = 0;

    void initializePipelines() {
        effects.resize(2);

        auto& gradient = effects[0];
        auto& sky      = effects[1];

        // ----------------------- layouts ---------------------- //
        VkDescriptorSetLayoutBinding binding {
            .binding         = 0,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT,
        };
        VkPushConstantRange pushConstantRange {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset     = 0,
            .size       = sizeof(PushConstants),
        };
        auto descriptorSetLayout = backend.createDescriptorSetLayout(&binding, 1).value();
        auto pipelineLayout      = backend.createPipelineLayout(1, &descriptorSetLayout, 1, &pushConstantRange).value();

        // ----------------------- shaders ---------------------- //
        auto gradientShader = backend.createShader("shaders/gradient.comp.spv").value();
        auto skyShader      = backend.createShader("shaders/sky.comp.spv").value();

        // ------------------- descriptor sets ------------------ //
        vector<VkDescriptorPoolSize> sizes = {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};
        auto dsPool                        = backend.createDescriptorSetPool(sizes).value();

        DescriptorSet descriptorSet;
        backend.allocateDescriptorSets(dsPool, 1, &descriptorSetLayout, &descriptorSet);

        VkDescriptorImageInfo imageInfo {
            .sampler     = nullptr,
            .imageView   = drawImage.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        VkWriteDescriptorSet write {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = descriptorSet,
            .dstBinding      = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = &imageInfo,
        };
        vkUpdateDescriptorSets(device.handle, 1, &write, 0, nullptr);

        // ------------------------ final ----------------------- //
        gradient = {
            .name          = "Gradient",
            .pipeline      = backend.createComputePipeline(gradientShader, pipelineLayout).value(),
            .descriptorSet = descriptorSet,
        };
        sky = {
            .name          = "Sky",
            .pipeline      = backend.createComputePipeline(skyShader, pipelineLayout).value(),
            .descriptorSet = descriptorSet,
        };
    }

  public:
    TestWindow(ezWindowCreateInfo createInfo) : ezWindow(createInfo) {};

    void onOpen() override {
        initializePipelines();
    }
    void onUpdate() override {
        auto& effect = effects[chosenPipeline];
        auto cmd     = currentFrameData->commandBuffer.handle;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline.handle);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline.layout.handle, 0, 1, &effect.descriptorSet, 0, nullptr);

        vkCmdPushConstants(cmd, effect.pipeline.layout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &effect.data);

        vkCmdDispatch(cmd, std::ceil(drawImage.extent.width / 16.f), std::ceil(drawImage.extent.height / 16.f), 1);

        drawGUI();
    }

    void drawGUI() {
        auto& effect = effects[chosenPipeline];

        ImGui::Begin("Pipelines");
        if (ImGui::BeginCombo("pipeline", effect.name)) {
            for (auto i = 0; i < effects.size(); i++) {
                if (ImGui::Selectable(effects[i].name, chosenPipeline == i)) {
                    chosenPipeline = i;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::InputFloat4("data1", (float*)&effect.data.data1);
        ImGui::InputFloat4("data2", (float*)&effect.data.data2);
        ImGui::InputFloat4("data3", (float*)&effect.data.data3);
        ImGui::InputFloat4("data4", (float*)&effect.data.data4);
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