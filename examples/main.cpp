#include"enginez/engine.hpp"
#include "enginez/graphics/graphics_backend.hpp"
// #include "enginez/graphics/graphics_backend.hpp"



int main(){
    enginez::EngineCreateInfo createInfo = {
        enginez::graphics::VULKAN
    };
    enginez::Engine engine(createInfo);
    engine.init();

    enginez::graphics::GraphicsBackend* gfx = engine.graphicsBackend.get();

    // auto window = gfx->createWindow("Test window", 600, 600);
    // auto window2 = gfx->createWindow("Test window2", 100, 600);
    auto memoryBlock = gfx->allocateMemory(100);
    auto shader = gfx->createShader("./vert.spv");
    auto buffer = gfx->createBuffer(50, enginez::graphics::R_BUFFER, memoryBlock, 0);

    gfx->cleanUpShader(shader);
    gfx->cleanUpMemoryBlock(memoryBlock);

    engine.loop();

    engine.cleanUp();

    // engine.logger.info("asd");
}