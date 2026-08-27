#include "enginez/ez_engine.hpp"
#include "logz/logger.hpp"
#include "enginez/graphics/ez_vulkan_backend.hpp"
#include <cstdint>
#include <format>
#include <memory>

using namespace enginez;
using namespace std;

ezEngine::ezEngine(EngineCreateInfo createInfo) {
    setupLogger();

    graphicsBackend.init(createInfo.deviceQueues);
}

void ezEngine::setupLogger(){
    logger.addConsoleSink(true, logz::DEBUG);
    logger.addFileSink("log.txt", logz::DEBUG);

    fastLogger.addFileSink("frameRate.csv", logz::DEBUG);
}

void ezEngine::init() {
}

void ezEngine::cleanUp() {
    graphicsBackend.cleanUp();
    logger.info("cleaned up the engine");
}

void ezEngine::loop(){
    auto startTime = chrono::high_resolution_clock::now();
    auto frameTime = chrono::high_resolution_clock::now() - startTime;

    uint64_t count = 0;

    logger.info("started engine loop.");

    while (!shouldTerminate){
        graphicsBackend.update();
        count++;
        frameTime = chrono::high_resolution_clock::now() - startTime;
        if(frameTime.count() > 1000000000){
            // logger.info(format("{} frames -> frame time: {}ns", count, 1000000000 / count));
            count = 0;
            startTime = chrono::high_resolution_clock::now();
        }
    }
}

void ezEngine::terminate() {
    shouldTerminate = true;
}