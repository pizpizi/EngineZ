
#pragma once
#include <cstddef>

namespace enginez::graphics {
    struct Buffer {
      public:
        virtual void cleanUp() = 0;
        virtual void upload(void* ptr, size_t size, size_t offset) = 0;
        virtual void download(void* ptr, size_t size, size_t offset) = 0;

      protected:
        bool cleanedUp = false;
    };
} // namespace enginez::graphics