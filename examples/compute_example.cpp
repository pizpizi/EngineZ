#include "enginez/graphics/pipelines.hpp"
#include "enginez/graphics/vulkan_backend.hpp"
#include "logz/logger.hpp"
#include <charconv>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <sys/types.h>
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
    logz::DefaultLogger& logger = logz::createDefaultLogger(logz::SINCE_PROGRAM_START, "main");
    logger.addConsoleSink(true, logz::DEBUG);
    logger.error("asd");

    VulkanBackend backend;

    std::vector<Queue> queues = {{.type = COMPUTE}, {.type = COMPUTE}};

    backend.init(queues);

    auto computeQueue = queues[0];
    auto computeQueue2 = queues[1];

    std::cout << computeQueue.handle << " " << computeQueue2.handle << '\n';

    DescriptorSetLayoutBinding setLayoutBindings[] = {{
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

    VkPushConstantRange pushConstants[] = {{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(uint),
    }};

    auto layout = backend.createDescriptorSetLayout(setLayoutBindings, 2).value();
    auto pipelineLayout = backend.createPipelineLayout(1, &layout, 1, pushConstants).value();
    auto shader = backend.createShader("shaders/compute.comp.spv").value();
    auto pipeline = backend.createComputePipeline(shader, pipelineLayout).value();

    auto descriptorSetPool =
        backend.createDescriptorSetPool({{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2}}, 1).value();

    DescriptorSet descriptorSet;
    backend.allocateDescriptorSets(descriptorSetPool, 1, &layout, &descriptorSet);

    size_t inputSize = sizeof(InputData);
    size_t outputSize = sizeof(OutputData);

    auto inputMemory = backend.allocateMemory(inputSize).value();
    auto outputMemory = backend.allocateMemory(outputSize).value();

    auto inputBuffer = backend.createBuffer(inputSize, R_BUFFER, inputMemory, 0).value();
    auto outputBuffer = backend.createBuffer(outputSize, RW_BUFFER, outputMemory, 0).value();

    InputData myData = {};
    for (int i = 0; i < 16; i++) {
        myData.A[i] = i * i;
        myData.B[i] = i * i * i;
    }
    backend.uploadToMemory(inputMemory, &myData, inputSize, 0);

    auto commandPool = backend.createCommandPool(computeQueue2).value();
    auto commandBuffer = backend.allocateCommandBuffer(commandPool).value();

    VkDescriptorBufferInfo inputInfo{.buffer = inputBuffer.handle, .offset = 0, .range = inputSize};
    VkDescriptorBufferInfo outputInfo{.buffer = outputBuffer.handle, .offset = 0, .range = outputSize};

    std::vector<VkWriteDescriptorSet> setWrites = {{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
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
                                                    .pBufferInfo = &outputInfo}};

    backend.updateDescriptorSets(setWrites, {});

    uint32_t arraySize = 16;

    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer.handle, &commandBufferBeginInfo);

    vkCmdBindPipeline(commandBuffer.handle, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.handle);
    vkCmdBindDescriptorSets(commandBuffer.handle, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout.handle, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdPushConstants(commandBuffer.handle, pipelineLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &arraySize);

    vkCmdDispatch(commandBuffer.handle, 1, 1, 1);

    vkEndCommandBuffer(commandBuffer.handle);

    backend.submitAndSynchronize(commandBuffer, computeQueue);

    // 6. Download Results
    OutputData result;
    backend.downloadFromMemory(outputMemory, &result, outputSize, 0);

    std::cout << "\n--- Input Array A ---" << '\n';
    for (int i = 0; i < 16; i++) {
        std::cout << std::setw(5) << myData.A[i] << " ";
        if ((i + 1) % 4 == 0) std::cout << '\n';
    }

    std::cout << "\n--- Input Array B ---" << '\n';
    for (int i = 0; i < 16; i++) {
        std::cout << std::setw(5) << myData.B[i] << " ";
        if ((i + 1) % 4 == 0) std::cout << '\n';
    }

    std::cout << "\n--- Output Array C (A + B) ---" << '\n';
    for (int i = 0; i < 16; i++) {
        std::cout << std::setw(5) << result.C[i] << " ";
        if ((i + 1) % 4 == 0) std::cout << '\n';
    }

    backend.cleanUp();
}