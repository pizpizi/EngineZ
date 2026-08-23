#pragma once
#include "logz/logger.hpp"
#include "graphics/ez_vulkan_backend.hpp"
#include <memory>
#include <vector>

namespace enginez {

    struct EngineCreateInfo {
      std::vector<graphics::Queue>& deviceQueues;
    };
    
    class ezEngine {
      public:
        logz::DefaultLogger& logger = logz::createDefaultLogger(logz::SINCE_PROGRAM_START, "EnginZ");
        logz::FastLogger& fastLogger = logz::createFastLogger("EngineZ");

        graphics::ezVulkanBackend graphicsBackend;

        ezEngine(EngineCreateInfo createInfo);
        // ~Engine();

        void init();
        void loop();
        void cleanUp();
        void terminate();


      private:
        bool shouldTerminate = false;
        void setupLogger();
    };
}; // namespace enginez