#include"enginez/engine.hpp"
// #include "enginez/graphics/graphics_backend.hpp"



int main(){
    enginez::EngineCreateInfo createInfo = {
        enginez::graphics::VULKAN
    };
    enginez::Engine engine(createInfo);
    engine.init();
    auto window = engine.graphicsBackend->createWindow("Test window", 600, 600);
    auto window2 = engine.graphicsBackend->createWindow("Test window2", 100, 600);
    auto buffer = engine.graphicsBackend->createBuffer(100);

    engine.loop();

    engine.cleanUp();

    // engine.logger.info("asd");
}