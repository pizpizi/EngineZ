#pragma once

#include "enginez_window.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace enginez {
    class Engine;
}
namespace enginez::graphics {
    enum GraphicsBackendType { VULKAN, OPENGL };
    enum ResourceType {
      
    };

    typedef uint32_t PipelineHandle;
    typedef uint32_t ShaderId;
    typedef uint32_t BufferId;
    typedef uint32_t MemoryBlockId;
    enum BufferType {
      VERTEX,
      INDEX,
      RW_BUFFER,
      R_BUFFER
    };

    class GraphicsBackend {
      public:
        virtual void init() = 0;
        virtual void cleanUp() = 0;

        virtual EnginezWindow* createWindow(std::string title, int width, int height) = 0;

        /* returns a non zero integer if successfull*/
        virtual MemoryBlockId allocateMemory(size_t size) = 0;
        virtual void downloadFromMemory(MemoryBlockId srcId, void* dst, size_t size, size_t offset) = 0;
        virtual void uploadToMemory(MemoryBlockId dstId, void* src, size_t size, size_t offset) = 0;
        virtual void cleanUpMemoryBlock(MemoryBlockId memoryBlock) = 0;

        virtual ShaderId createShader(const char* filePath) = 0;
        virtual void cleanUpShader(ShaderId shader) = 0;

        virtual BufferId createBuffer(size_t size, BufferType type, MemoryBlockId boundMemoryId, size_t offset) = 0;
        virtual void cleanUpBuffer(BufferId id) = 0;

        virtual PipelineHandle createComputePipeline(ShaderId computeShader) = 0;

      protected:
        friend enginez::Engine;
        std::vector<EnginezWindow*> windows;
    };

}; // namespace enginez::graphics