#pragma once

#include "buffer.hpp"
#include "enginez_window.hpp"
#include <cstddef>
#include <vector>

namespace enginez {
    class Engine;
}
namespace enginez::graphics {
    enum GraphicsBackendType { VULKAN, OPENGL };

    class GraphicsBackend {
      public:
        virtual void init() = 0;
        virtual void cleanUp() = 0;

        virtual EnginezWindow* createWindow(std::string title, int width, int height) = 0;
        virtual Buffer* createBuffer(size_t size)=0;

      protected:
        friend enginez::Engine;
        std::vector<EnginezWindow*> windows;
        std::vector<Buffer*> buffers;
    };

}; // namespace enginez::graphics