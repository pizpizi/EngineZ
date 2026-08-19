#pragma once
#include "logz/logger.hpp"
#include "graphics/vulkan_backend.hpp"
#include <memory>
#include <vector>

namespace enginez {

    struct EngineCreateInfo {
      std::vector<graphics::Queue>& deviceQueues;
    };
    
    class Engine {
      public:
        logz::DefaultLogger& logger = logz::createDefaultLogger(logz::SINCE_PROGRAM_START, "EnginZ");
        logz::FastLogger& fastLogger = logz::createFastLogger("EngineZ");

        std::unique_ptr<graphics::VulkanBackend> graphicsBackend;

        Engine(EngineCreateInfo createInfo);
        // ~Engine();

        void init();
        void loop();
        void cleanUp();

      private:

        void setupLogger();
    };
}; // namespace enginez