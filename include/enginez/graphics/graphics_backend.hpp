#pragma once

#include "enginez/graphics/enginez_window.hpp"
#include <memory>
#include <vector>

namespace enginez::graphics {

    enum GraphicsBackendType { VULKAN, OPENGL };

    class GraphicsBackend {
      public:
        virtual void init() = 0;
        virtual void cleanUp() = 0;
        virtual EnginezWindow* createWindow(std::string title, int width, int height) = 0;

        std::vector<std::unique_ptr<EnginezWindow>> windows;
    };

}; // namespace enginez::graphics