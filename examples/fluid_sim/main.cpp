#include "enginez/ez_engine.hpp"
#include "enginez/graphics/ez_types.hpp"
#include "enginez/graphics/ez_window.hpp"
#include "fluid_sim_window.hpp"
#include <vector>


using namespace std;

vector<Queue> queues = {{.type = COMPUTE}, {.type = GRAPHICS}, {.type = PRESENT}};

int main() {

    EngineCreateInfo engineCreateInfo {
        .deviceQueues = queues,
    };

    ezEngine engine(engineCreateInfo);

    ezWindowCreateInfo windowCreateInfo {
        .graphicsQueue = queues[1],
        .presentQueue  = queues[2],
        .engine        = engine,
        .title         = "Fluid Sim",
        .width         = 800,
        .height        = 800,
    };

    FluidSimWindow window(windowCreateInfo, queues[0]);

    engine.graphicsBackend.addWindow(&window);
    engine.loop();
}