#pragma once
#include "graphics/graphics_backend.hpp"
#include "logz/logger.hpp"
#include <memory>

namespace enginez {

    struct EngineCreateInfo {
        graphics::GraphicsBackendType graphicsBackendType;
    };
    class Engine {
      public:
        logz::DefaultLogger& logger = logz::createDefaultLogger(logz::SINCE_PROGRAM_START, "EnginZ");
        logz::FastLogger& fastLogger = logz::createFastLogger("EngineZ");

        std::unique_ptr<graphics::GraphicsBackend> graphicsBackend;

        Engine(EngineCreateInfo createInfo);
        // ~Engine();

        void init();
        void loop();
        void cleanUp();

      private:

        void setupLogger();
    };
}; // namespace enginez