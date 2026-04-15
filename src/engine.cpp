#include "enginez/engine.hpp"
#include "logz/logger.hpp"
#include "enginez/graphics/vulkan_backend.hpp"
#include <cstdint>
#include <format>
#include <memory>

using namespace enginez;
using namespace std;

Engine::Engine(EngineCreateInfo createInfo) {
    setupLogger();

    logger.info("setup the main logger");
    graphicsBackend = std::make_unique<graphics::VulkanBackend>();
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
    auto startTime = chrono::high_resolution_clock::now();
    auto frameTime = chrono::high_resolution_clock::now() - startTime;

    uint64_t count = 0;

    while (true){
        graphicsBackend->update();
        count++;
        frameTime = chrono::high_resolution_clock::now() - startTime;
        if(frameTime.count() > 1000000000){
            logger.info(format("{} frames -> frame time: {}ns", count, 1000000000 / count));
            count = 0;
            startTime = chrono::high_resolution_clock::now();
        }
    }
}