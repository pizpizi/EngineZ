#include "enginez/engine.hpp"
#include "GLFW/glfw3.h"
#include "enginez/graphics/enginez_window.hpp"
#include "enginez/graphics/graphics_backend.hpp"
#include "logz/logger.hpp"
#include "vulkan/vulkan_backend.hpp"
#include <cstdint>
#include <format>
#include <memory>

using namespace enginez;
using namespace std;

Engine::Engine(EngineCreateInfo createInfo) {
    setupLogger();

    logger.info("setup the main logger");
    if (createInfo.graphicsBackendType == graphics::VULKAN) {
        graphicsBackend = std::make_unique<graphics::VulkanBackend>();
    }
}

void Engine::setupLogger(){
    logger.addConsoleSink(true, logz::DEBUG);
    logger.addFileSink("log.txt", logz::DEBUG);

    fastLogger.addFileSink("frameRate.csv", logz::DEBUG);
}

void Engine::init() {
    graphicsBackend->init();
}

void Engine::cleanUp() {
    graphicsBackend->cleanUp();
    logger.info("cleaned up the engine");
}

void Engine::loop(){
    int winNum = graphicsBackend->windows.size();

    auto startTime = chrono::high_resolution_clock::now();
    auto frameTime = chrono::high_resolution_clock::now() - startTime;

    uint64_t count = 0;

    fastLogger.log("frame_time");
    while (true){
        glfwPollEvents();
        winNum = graphicsBackend->windows.size();
        if(winNum == 0) break;

        for (int i = graphicsBackend->windows.size()-1; i >= 0; i--) {
            auto& win = graphicsBackend->windows[i];
            win->update();
            
            if(win->getClosed()){
                win->cleanUp();
                graphicsBackend->windows.erase(graphicsBackend->windows.begin() + i);
            }
        }
        count++;
        frameTime = chrono::high_resolution_clock::now() - startTime;
        if(frameTime.count() > 1000000000){
            logger.info(format("{} frames -> frame time: {}ns", count, 1000000000 / count));
            count = 0;
            startTime = chrono::high_resolution_clock::now();
        }
    }
}