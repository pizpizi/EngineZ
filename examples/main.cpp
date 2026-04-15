#include "enginez/engine.hpp"
#include "enginez/graphics/pipelines.hpp"
#include "enginez/graphics/vulkan_backend.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

struct InputData {
    float A[16];
    float B[16];
};

struct OutputData {
    float C[16];
};

using namespace enginez::graphics;
int main() {
    enginez::EngineCreateInfo createInfo = {};
    enginez::Engine engine(createInfo);
    engine.init();

    enginez::graphics::VulkanBackend* gfx = engine.graphicsBackend.get();

    VkDescriptorSetLayoutBinding setLayoutBindings[] = {{
                                                            .binding = 0,
                                                            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                            .descriptorCount = 1,
                                                            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                                                        },
                                                        {
                                                            .binding = 1,
                                                            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                                            .descriptorCount = 1,
                                                            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                                                        }};

    auto layout = gfx->createDescriptorSetLayout(setLayoutBindings, 2).value();
    auto pipelineLayout = gfx->createPipelineLayout(1, &layout).value();
    auto shader = gfx->createShader("./examples/shaders/test2.spv").value();
    auto pipeline = gfx->createComputePipeline(shader, pipelineLayout).value();

    auto descriptorSetPool =
        gfx->createDescriptorSetPool({{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2}}, 1).value();

    DescriptorSet descriptorSet;
    gfx->allocateDescriptorSets(descriptorSetPool, 1, &layout, &descriptorSet);

    size_t inputSize = sizeof(InputData);
    size_t outputSize = sizeof(OutputData);

    auto inputMemory = gfx->allocateMemory(inputSize).value();
    auto outputMemory = gfx->allocateMemory(outputSize).value();

    auto inputBuffer = gfx->createBuffer(inputSize, R_BUFFER, inputMemory, 0).value();
    auto outputBuffer = gfx->createBuffer(outputSize, RW_BUFFER, outputMemory, 0).value();

    InputData myData = {};
    for (int i = 0; i < 16; i++) {
        myData.A[i] = 1.0f;
        myData.B[i] = 2.0f;
    }
    gfx->uploadToMemory(inputMemory, &myData, inputSize, 0);

    auto commandPool = gfx->createCommandPool().value();
    auto commandBuffer = gfx->allocateCommandBuffer(commandPool).value();


    VkDescriptorBufferInfo inputInfo{.buffer = inputBuffer.handle, .offset = 0, .range = inputSize};
    VkDescriptorBufferInfo outputInfo{.buffer = outputBuffer.handle, .offset = 0, .range = outputSize};

    std::vector<VkWriteDescriptorSet> setWrites =
    { {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .dstSet = descriptorSet,
       .dstBinding = 0,
       .dstArrayElement = 0,
       .descriptorCount = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .pBufferInfo = &inputInfo},
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .dstSet = descriptorSet,
       .dstBinding = 1,
       .dstArrayElement = 0,
       .descriptorCount = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
       .pBufferInfo = &outputInfo} };

    gfx->updateDescriptorSets(setWrites, {});


    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer.handle, &commandBufferBeginInfo);

    vkCmdBindPipeline(commandBuffer.handle, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.handle);
    vkCmdBindDescriptorSets(commandBuffer.handle, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout.handle, 0, 1, &descriptorSet, 0, nullptr);

    vkCmdDispatch(commandBuffer.handle, 1, 1, 1);
    
    vkEndCommandBuffer(commandBuffer.handle);

    gfx->submitAndSynchronize(commandBuffer);

    // 6. Download Results
    OutputData result;
    gfx->downloadFromMemory(outputMemory, &result, outputSize, 0);

std::cout << "\n--- Input Array A ---" << std::endl;
    for (int i = 0; i < 16; i++) {
        std::cout << std::setw(5) << myData.A[i] << " ";
        if ((i + 1) % 4 == 0) std::cout << std::endl;
    }

    std::cout << "\n--- Input Array B ---" << std::endl;
    for (int i = 0; i < 16; i++) {
        std::cout << std::setw(5) << myData.B[i] << " ";
        if ((i + 1) % 4 == 0) std::cout << std::endl;
    }

    std::cout << "\n--- Output Array C (A + B) ---" << std::endl;
    for (int i = 0; i < 16; i++) {
        std::cout << std::setw(5) << result.C[i] << " ";
        if ((i + 1) % 4 == 0) std::cout << std::endl;
    }

    engine.cleanUp();
}